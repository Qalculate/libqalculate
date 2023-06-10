/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

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

/*
	Functions for simple handling of interval variables
*/

#define IS_VAR_EXP(x, ofa) ((x.isVariable() && x.variable()->isKnown() && (!ofa || x.variable()->title() == "\b")) || (x.isPower() && x[0].isVariable() && x[0].variable()->isKnown() && (!ofa || x[0].variable()->title() == "\b")))

void factorize_variable(MathStructure &mstruct, const MathStructure &mvar, bool deg2) {
	if(deg2) {
		// ax^2+bx = (sqrt(b)*x+(a/sqrt(b))/2)^2-((a/sqrt(b))/2)^2
		MathStructure a_struct, b_struct, mul_struct(1, 1, 0);
		for(size_t i2 = 0; i2 < mstruct.size();) {
			bool b = false;
			if(mstruct[i2] == mvar) {
				a_struct.set(1, 1, 0);
				b = true;
			} else if(mstruct[i2].isPower() && mstruct[i2][0] == mvar && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo()) {
				b_struct.set(1, 1, 0);
				b = true;
			} else if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(mstruct[i2][i3] == mvar) {
						a_struct = mstruct[i2];
						a_struct.delChild(i3 + 1);
						b = true;
						break;
					} else if(mstruct[i2][i3].isPower() && mstruct[i2][i3][0] == mvar && mstruct[i2][i3][1].isNumber() && mstruct[i2][i3][1].number().isTwo()) {
						b_struct = mstruct[i2];
						b_struct.delChild(i3 + 1);
						b = true;
						break;
					}
				}
			}
			if(b) {
				mstruct.delChild(i2 + 1);
			} else {
				i2++;
			}
		}
		if(b_struct == a_struct) {
			if(a_struct.isMultiplication() && a_struct.size() == 1) mul_struct = a_struct[0];
			else mul_struct = a_struct;
			a_struct.set(1, 1, 0);
			b_struct.set(1, 1, 0);
		} else if(b_struct.isMultiplication() && a_struct.isMultiplication()) {
			size_t i3 = 0;
			for(size_t i = 0; i < a_struct.size();) {
				bool b = false;
				for(size_t i2 = i3; i2 < b_struct.size(); i2++) {
					if(a_struct[i] == b_struct[i2]) {
						i3 = i2;
						if(mul_struct.isOne()) mul_struct = a_struct[i];
						else mul_struct.multiply(a_struct[i], true);
						a_struct.delChild(i + 1);
						b_struct.delChild(i2 + 1);
						b = true;
						break;
					}
				}
				if(!b) i++;
			}
		}
		if(a_struct.isMultiplication() && a_struct.size() == 0) a_struct.set(1, 1, 0);
		else if(a_struct.isMultiplication() && a_struct.size() == 1) a_struct.setToChild(1);
		if(b_struct.isMultiplication() && b_struct.size() == 0) b_struct.set(1, 1, 0);
		else if(b_struct.isMultiplication() && b_struct.size() == 1) b_struct.setToChild(1);
		if(!b_struct.isOne()) {
			b_struct.raise(nr_half);
			a_struct.divide(b_struct);
		}
		a_struct.multiply(nr_half);
		if(b_struct.isOne()) b_struct = mvar;
		else b_struct *= mvar;
		b_struct += a_struct;
		b_struct.raise(nr_two);
		a_struct.raise(nr_two);
		a_struct.negate();
		b_struct += a_struct;
		if(!mul_struct.isOne()) b_struct *= mul_struct;
		if(mstruct.size() == 0) mstruct = b_struct;
		else mstruct.addChild(b_struct);
	} else {
		vector<MathStructure*> left_structs;
		for(size_t i2 = 0; i2 < mstruct.size();) {
			bool b = false;
			if(mstruct[i2] == mvar) {
				mstruct[i2].set(1, 1, 0, true);
				b = true;
			} else if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(mstruct[i2][i3] == mvar) {
						mstruct[i2].delChild(i3 + 1, true);
						b = true;
						break;
					}
				}
			}
			if(b) {
				i2++;
			} else {
				mstruct[i2].ref();
				left_structs.push_back(&mstruct[i2]);
				mstruct.delChild(i2 + 1);
			}
		}
		mstruct.multiply(mvar);
		for(size_t i = 0; i < left_structs.size(); i++) {
			mstruct.add_nocopy(left_structs[i], true);
			mstruct.evalSort(false);
		}
	}
}
bool var_contains_interval(const MathStructure &mstruct) {
	if(mstruct.isNumber()) return mstruct.number().isInterval();
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_INTERVAL || mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY)) return true;
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) return var_contains_interval(((KnownVariable*) mstruct.variable())->get());
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(var_contains_interval(mstruct[i])) return true;
	}
	return false;
}

bool factorize_variables(MathStructure &mstruct, const EvaluationOptions &eo, bool ofa = false) {
	bool b = false;
	if(mstruct.type() == STRUCT_ADDITION) {
		vector<MathStructure> variables;
		vector<size_t> variable_count;
		vector<int> term_sgn;
		vector<bool> term_deg2;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(CALCULATOR->aborted()) break;
					if(IS_VAR_EXP(mstruct[i][i2], ofa)) {
						bool b_found = false;
						for(size_t i3 = 0; i3 < variables.size(); i3++) {
							if(variables[i3] == mstruct[i][i2]) {
								variable_count[i3]++;
								b_found = true;
								if(term_sgn[i3] == 1 && !mstruct[i].representsNonNegative(true)) term_sgn[i3] = 0;
								else if(term_sgn[i3] == -1 && !mstruct[i].representsNonPositive(true)) term_sgn[i3] = 0;
								break;
							}
						}
						if(!b_found) {
							variables.push_back(mstruct[i][i2]);
							variable_count.push_back(1);
							term_deg2.push_back(false);
							if(mstruct[i].representsNonNegative(true)) term_sgn.push_back(1);
							else if(mstruct[i].representsNonPositive(true)) term_sgn.push_back(-1);
							else term_sgn.push_back(0);
						}
					}
				}
			} else if(IS_VAR_EXP(mstruct[i], ofa)) {
				bool b_found = false;
				for(size_t i3 = 0; i3 < variables.size(); i3++) {
					if(variables[i3] == mstruct[i]) {
						variable_count[i3]++;
						b_found = true;
						if(term_sgn[i3] == 1 && !mstruct[i].representsNonNegative(true)) term_sgn[i3] = 0;
						else if(term_sgn[i3] == -1 && !mstruct[i].representsNonPositive(true)) term_sgn[i3] = 0;
						break;
					}
				}
				if(!b_found) {
					variables.push_back(mstruct[i]);
					variable_count.push_back(1);
					term_deg2.push_back(false);
					if(mstruct[i].representsNonNegative(true)) term_sgn.push_back(1);
					else if(mstruct[i].representsNonPositive(true)) term_sgn.push_back(-1);
					else term_sgn.push_back(0);
				}
			}
		}
		for(size_t i = 0; i < variables.size();) {
			bool b_erase = false;
			if(!var_contains_interval(variables[i])) {
				b_erase = true;
			} else if(variable_count[i] == 1 || term_sgn[i] != 0) {
				b_erase = true;
				if(!variables[i].isPower() || (variables[i][1].isNumber() && variables[i][1].number().isTwo())) {
					for(size_t i2 = i + 1; i2 < variables.size(); i2++) {
						if((variables[i].isPower() && !variables[i2].isPower() && variables[i][0] == variables[i2])|| (!variables[i].isPower() && variables[i2].isPower() && variables[i2][0] == variables[i] && variables[i2][1].isNumber() && variables[i2][1].number().isTwo())) {
							bool b_erase2 = false;
							if(variable_count[i2] == 1) {
								if(term_sgn[i] == 0 || term_sgn[i2] != term_sgn[i]) {
									if(variable_count[i] == 1) {
										term_deg2[i] = true;
										variable_count[i] = 2;
										term_sgn[i] = 0;
										if(variables[i].isPower()) variables[i].setToChild(1);
									} else {
										term_sgn[i] = 0;
									}
									b_erase = false;
								}
								b_erase2 = true;
							} else if(term_sgn[i2] != 0) {
								if(term_sgn[i] == 0 || term_sgn[i2] != term_sgn[i]) {
									if(variable_count[i] != 1) {
										term_sgn[i] = 0;
										b_erase = false;
									}
									term_sgn[i2] = 0;
								} else {
									b_erase2 = true;
								}
							}
							if(b_erase2) {
								variable_count.erase(variable_count.begin() + i2);
								variables.erase(variables.begin() + i2);
								term_deg2.erase(term_deg2.begin() + i2);
								term_sgn.erase(term_sgn.begin() + i2);
							}
							break;
						}
					}
				}
			}
			if(b_erase) {
				variable_count.erase(variable_count.begin() + i);
				variables.erase(variables.begin() + i);
				term_deg2.erase(term_deg2.begin() + i);
				term_sgn.erase(term_sgn.begin() + i);
			} else if(variable_count[i] == mstruct.size()) {
				factorize_variable(mstruct, variables[i], term_deg2[i]);
				if(CALCULATOR->aborted()) return true;
				factorize_variables(mstruct, eo, ofa);
				return true;
			} else {
				i++;
			}
		}
		if(variables.size() == 1) {
			factorize_variable(mstruct, variables[0], term_deg2[0]);
			if(CALCULATOR->aborted()) return true;
			factorize_variables(mstruct, eo, ofa);
			return true;
		}
		Number uncertainty;
		size_t u_index = 0;
		for(size_t i = 0; i < variables.size(); i++) {
			const MathStructure *v_ms;
			Number nr;
			if(variables[i].isPower()) v_ms = &((KnownVariable*) variables[i][0].variable())->get();
			else v_ms = &((KnownVariable*) variables[i].variable())->get();
			if(v_ms->isNumber()) nr = v_ms->number();
			else if(v_ms->isMultiplication() && v_ms->size() > 0 && (*v_ms)[0].isNumber()) nr = (v_ms)[0].number();
			else {
				MathStructure mtest(*v_ms);
				mtest.unformat(eo);
				mtest.calculatesub(eo, eo, true);
				if(mtest.isNumber()) nr = mtest.number();
				else if(mtest.isMultiplication() && mtest.size() > 0 && mtest[0].isNumber()) nr = mtest[0].number();
			}
			if(nr.isInterval()) {
				Number u_candidate(nr.uncertainty());
				if(variables[i].isPower() && variables[i][1].isNumber() && variables[i][1].number().isReal()) u_candidate.raise(variables[i][1].number());
				u_candidate.multiply(variable_count[i]);
				if(u_candidate.isGreaterThan(uncertainty)) {
					uncertainty = u_candidate;
					u_index = i;
				}
			}
		}
		if(!uncertainty.isZero()) {
			factorize_variable(mstruct, variables[u_index], term_deg2[u_index]);
			if(CALCULATOR->aborted()) return true;
			factorize_variables(mstruct, eo, ofa);
			return true;
		}

	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(factorize_variables(mstruct[i], eo, ofa)) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
		if(CALCULATOR->aborted()) return b;
	}
	return b;
}
bool contains_duplicate_interval_variables_eq(const MathStructure &mstruct, const MathStructure &xvar, vector<KnownVariable*> &vars) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown() && ((KnownVariable*) mstruct.variable())->get().containsInterval(false, true, false, false)) {
		KnownVariable *v = (KnownVariable*) mstruct.variable();
		for(size_t i = 0; i < vars.size(); i++) {
			if(vars[i] == v) {
				return true;
			}
		}
		vars.push_back(v);
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_duplicate_interval_variables_eq(mstruct[i], xvar, vars)) return true;
	}
	return false;
}
bool fix_eqs(MathStructure &m, const EvaluationOptions &eo) {
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_eqs(m[i], eo)) m.childUpdated(i + 1);
	}
	if(m.isComparison()) {
		if(CALCULATOR->aborted()) return false;
		const MathStructure *x_var;
		if(eo.isolate_var && m.contains(*eo.isolate_var)) x_var = eo.isolate_var;
		else x_var = &m.find_x_var();
		if(!x_var->isUndefined()) {
			vector<KnownVariable*> vars;
			if(contains_duplicate_interval_variables_eq(m, *x_var, vars)) {
				if(!m[0].contains(*x_var)) {
					m.swapChildren(1, 2);
				} else if(m[0].isAddition()) {
					for(size_t i = 0; i < m[0].size();) {
						if(!m[0][i].contains(*x_var)) {
							m[0][i].calculateNegate(eo);
							m[0][i].ref();
							m[1].add_nocopy(&m[0][i], true);
							m[1].calculateAddLast(eo);
							m[0].delChild(i + 1);
						} else {
							i++;
						}
					}
					if(m[0].size() == 1) m[0].setToChild(1, true);
					else if(m[0].size() == 0) m[0].clear(true);
					m.childrenUpdated();
				}
				if(m[1].isAddition()) {
					for(size_t i = 0; i < m[1].size();) {
						if(m[1][i].contains(*x_var)) {
							m[1][i].calculateNegate(eo);
							m[1][i].ref();
							m[0].add_nocopy(&m[1][i], true);
							m[0].calculateAddLast(eo);
							m[1].delChild(i + 1);
						} else {
							i++;
						}
					}
					if(m[1].size() == 1) m[1].setToChild(1, true);
					else if(m[1].size() == 0) m[1].clear(true);
					m.childrenUpdated();
				} else if(m[1].contains(*x_var)) {
					m[0].calculateSubtract(m[1], eo);
					m[1].clear(true);
				}
				vars.clear();
				if(m[0].containsType(STRUCT_ADDITION) && contains_duplicate_interval_variables_eq(m[0], *x_var, vars)) m[0].factorize(eo, false, false, 0, false, 1, NULL, m_undefined, false, false, 3);
				return true;
			}
		}
	}
	return false;
}
// end functions for simple handling of interval variables


/*

	Functions for handling of interval variables using interval arithmetic. Calculates the derivative of the expression for each interval variable, searches 	points where the derivative is zero, and calculates the expression separately for appropriate subintervals.

*/

void find_interval_variables(const MathStructure &mstruct, vector<KnownVariable*> &vars, vector<int> &v_count, vector<int> &v_prec) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		KnownVariable *v = (KnownVariable*) mstruct.variable();
		int var_prec = PRECISION + 11;
		const MathStructure &mv = v->get();
		for(size_t i = 0; i < vars.size(); i++) {
			if(vars[i] == v) {
				v_count[i]++;
				return;
			}
		}
		if(mv.isNumber()) {
			if(mv.number().isInterval()) var_prec = mv.number().precision(1);
			else if(CALCULATOR->usesIntervalArithmetic() && mv.number().precision() >= 0) var_prec = mv.number().precision();
		} else if(mv.isMultiplication()) {
			for(size_t i = 0; i < mv.size(); i++) {
				if(mv[i].isNumber()) {
					if(mv[i].number().isInterval()) {var_prec = mv[i].number().precision(1); break;}
					else if(mv[i].number().precision() >= 0) {var_prec = mv[i].number().precision(); break;}
				}
			}
		}
		if(var_prec <= PRECISION + 10) {
			bool b = false;
			for(size_t i = 0; i < v_prec.size(); i++) {
				if(var_prec < v_prec[i]) {
					v_prec.insert(v_prec.begin() + i, var_prec);
					v_count.insert(v_count.begin() + i, 1);
					vars.insert(vars.begin() + i, v);
					b = true;
					break;
				}
			}
			if(!b) {
				v_prec.push_back(var_prec);
				v_count.push_back(1);
				vars.push_back(v);
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		find_interval_variables(mstruct[i], vars, v_count, v_prec);
	}
}
bool contains_not_nonzero(MathStructure &m) {
	if(m.isNumber() && !m.number().isNonZero()) {
		return true;
	} else if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(contains_not_nonzero(m[i])) return true;
		}
	}
	return false;
}
bool contains_undefined(MathStructure &m, const EvaluationOptions &eo = default_evaluation_options, bool calc = false, const MathStructure &x_var = m_zero, const MathStructure &m_intval = nr_zero) {
	if(m.isPower() && (m[1].representsNegative() || (m[1].isNumber() && !m[1].number().isNonNegative()))) {
		if(calc) {
			m[0].replace(x_var, m_intval, true);
			m[0].calculatesub(eo, eo, true);
		}
		if(contains_not_nonzero(m[0])) return true;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_undefined(m[i], eo, calc, x_var, m_intval)) return true;
	}
	return false;
}

// search for values where the expression is non-zero, using intervals of diminishing width
bool find_interval_zeroes(const MathStructure &mstruct, MathStructure &malts, const MathStructure &mvar, const Number &nr_intval, const EvaluationOptions &eo, int depth, const Number &nr_prec, int orig_prec = 0, int is_real = -1, int undef_depth = 0) {
	if(CALCULATOR->aborted()) return false;
	if(depth == 0) orig_prec = nr_intval.precision(1);
	MathStructure mtest(mstruct);
	mtest.replace(mvar, nr_intval);
	mtest.eval(eo);
	if(is_real < 0) is_real = mtest.representsNonComplex(true);
	ComparisonResult cmp;
	if(is_real == 0) {
		MathStructure m_re(CALCULATOR->getFunctionById(FUNCTION_ID_RE), &mtest, NULL);
		m_re.calculateFunctions(eo);
		cmp = m_re.compare(m_zero);
		MathStructure m_im(CALCULATOR->getFunctionById(FUNCTION_ID_IM), &mtest, NULL);
		m_im.calculateFunctions(eo);
		ComparisonResult cmp2 = m_im.compare(m_zero);
		if(COMPARISON_IS_NOT_EQUAL(cmp) || cmp2 == COMPARISON_RESULT_EQUAL || cmp == COMPARISON_RESULT_UNKNOWN) cmp = cmp2;
	} else {
		cmp = mtest.compare(m_zero);
	}
	if(COMPARISON_IS_NOT_EQUAL(cmp)) {
		return true;
	} else if(cmp != COMPARISON_RESULT_UNKNOWN || (undef_depth <= 5 && contains_undefined(mtest))) {
		if(cmp == COMPARISON_RESULT_EQUAL || (nr_intval.precision(1) > (orig_prec > PRECISION ? orig_prec + 5 : PRECISION + 5) || (!nr_intval.isNonZero() && nr_intval.uncertainty().isLessThan(nr_prec)))) {
			if(cmp == COMPARISON_RESULT_EQUAL && depth <= 3) return false;
			if(malts.size() > 0 && (cmp = malts.last().compare(nr_intval)) != COMPARISON_RESULT_UNKNOWN && cmp != COMPARISON_RESULT_EQUAL && COMPARISON_MIGHT_BE_EQUAL(cmp)) {
				malts.last().number().setInterval(malts.last().number(), nr_intval);
				if(malts.last().number().precision(1) < (orig_prec > PRECISION ? orig_prec + 3 : PRECISION + 3)) {
					return false;
				}
			} else {
				malts.addChild(nr_intval);
			}
			return true;
		}
		vector<Number> splits;
		nr_intval.splitInterval(2, splits);
		for(size_t i = 0; i < splits.size(); i++) {
			if(!find_interval_zeroes(mstruct, malts, mvar, splits[i], eo, depth + 1, nr_prec, orig_prec, is_real, cmp == COMPARISON_RESULT_UNKNOWN ? undef_depth + 1 : 0)) return false;
		}
		return true;
	}
	return false;
}
bool contains_interval_variable(const MathStructure &m, int i_type = 0) {
	if(i_type == 0 && m.isVariable() && m.containsInterval(true, true, false, 1, false)) return true;
	else if(i_type == 1 && m.containsInterval(true, false, false, 1, true)) return true;
	else if(i_type == 2 && m.containsInterval(true, true, false, 1, true)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_interval_variable(m[i])) return true;
	}
	return false;
}
bool calculate_differentiable_functions(MathStructure &m, const EvaluationOptions &eo, bool recursive = true, bool do_unformat = true) {
	if(m.isFunction() && m.function() != eo.protected_function && function_differentiable(m.function())) {
		return m.calculateFunctions(eo, recursive, do_unformat);
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(calculate_differentiable_functions(m[i], eo, recursive, do_unformat)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
	}
	return b;
}
bool calculate_nondifferentiable_functions(MathStructure &m, const EvaluationOptions &eo, bool recursive, bool do_unformat, int i_type) {
	if(m.isFunction() && m.function() != eo.protected_function) {
		if((i_type <= 0 && (!function_differentiable(m.function()) || (m.function()->id() == FUNCTION_ID_INCOMPLETE_BETA && (m.size() != 3 || m[1].containsInterval(true, false, false, 1, true) || m[2].containsInterval(true, false, false, 1, true))) || (m.function()->id() == FUNCTION_ID_I_GAMMA && (m.size() != 2 || m[1].containsInterval(true, false, false, 1, true))))) || (i_type >= 0 && !contains_interval_variable(m, i_type))) {
			if(m.calculateFunctions(eo, false, do_unformat)) {
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			} else {
				return false;
			}
		} else if(m.function()->id() == FUNCTION_ID_ABS && m.size() == 1) {
			EvaluationOptions eo3 = eo;
			eo3.split_squares = false;
			eo3.assume_denominators_nonzero = false;
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !m.containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			m[0].calculatesub(eo3, eo);
			m.childUpdated(1);
			if(m[0].representsNegative(true)) {
				m.setToChild(1);
				m.negate();
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(m[0].representsNonNegative(true)) {
				m.setToChild(1);
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(m[0].isMultiplication()) {
				m.setToChild(1);
				for(size_t i = 0; i < m.size(); i++) {
					m[i].transformById(FUNCTION_ID_ABS);
				}
				m.childrenUpdated();
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(eo.approximation != APPROXIMATION_EXACT) {
				eo3.approximation = APPROXIMATION_APPROXIMATE;
				MathStructure mtest(m[0]);
				mtest.calculatesub(eo3, eo);
				if(mtest.representsNegative(true)) {
					m.setToChild(1);
					m.negate();
					if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
					return true;
				}
				if(mtest.representsNonNegative(true)) {
					m.setToChild(1);
					if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
					return true;
				}
			}
		}
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(calculate_nondifferentiable_functions(m[i], eo, recursive, do_unformat, i_type)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
	}
	return b;
}

void remove_nonzero_mul(MathStructure &msolve, const MathStructure &u_var, const EvaluationOptions &eo) {
	if(!msolve.isMultiplication()) return;
	for(size_t i = 0; i < msolve.size();) {
		if(!msolve[i].contains(u_var, true)) {
			msolve[i].eval(eo);
			if(msolve[i].representsNonZero(true)) {
				if(msolve.size() == 2) {
					msolve.delChild(i + 1, true);
					break;
				}
				msolve.delChild(i + 1, true);
			} else {
				remove_nonzero_mul(msolve[i], u_var, eo);
				i++;
			}
		} else {
			remove_nonzero_mul(msolve[i], u_var, eo);
			i++;
		}
	}
}

void solve_intervals2(MathStructure &mstruct, vector<KnownVariable*> vars, const EvaluationOptions &eo_pre) {
	if(vars.size() > 0) {
		EvaluationOptions eo = eo_pre;
		eo.approximation = APPROXIMATION_EXACT_VARIABLES;
		eo.expand = false;
		if(eo.calculate_functions) calculate_differentiable_functions(mstruct, eo);
		KnownVariable *v = vars[0];
		vars.erase(vars.begin());
		UnknownVariable *u_var = new UnknownVariable("", "u");
		Number nr_intval;
		MathStructure mvar(u_var);
		const MathStructure &mv = v->get();
		MathStructure mmul(1, 1, 0);
		if(mv.isMultiplication()) {
			for(size_t i = 0; i < mv.size(); i++) {
				if(mv[i].isNumber() && mv[i].number().isInterval()) {
					mmul = mv;
					mmul.delChild(i + 1, true);
					mvar.multiply(mmul);
					nr_intval = mv[i].number();
					u_var->setInterval(nr_intval);
					break;
				}
			}
		} else {
			nr_intval = mv.number();
			u_var->setInterval(mv);
		}
		MathStructure msolve(mstruct);
		msolve.replace(v, mvar);
		msolve.unformat(eo);
		bool b = true;
		CALCULATOR->beginTemporaryStopMessages();

		if(!msolve.differentiate(u_var, eo) || msolve.countTotalChildren(false) > 10000 || contains_diff_for(msolve, u_var) || CALCULATOR->aborted()) {
			b = false;
		}

		MathStructure malts;
		malts.clearVector();
		if(b) {
			eo.keep_zero_units = false;
			eo.approximation = APPROXIMATION_APPROXIMATE;
			eo.expand = eo_pre.expand;
			// test if derivative is non-zero, replacing each occurrence of the variable with independent intervals
			MathStructure mtest(msolve);
			mtest.replace(u_var, nr_intval);
			mtest.eval(eo);
			if(mtest.countTotalChildren(false) < 100) {
				if(mtest.representsNonComplex(true)) {
					ComparisonResult cmp = mtest.compare(m_zero);
					if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) b = false;
				} else {
					MathStructure m_re(CALCULATOR->getFunctionById(FUNCTION_ID_RE), &mtest, NULL);
					m_re.calculateFunctions(eo);
					ComparisonResult cmp = m_re.compare(m_zero);
					if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) {
						b = false;
					} else {
						MathStructure m_im(CALCULATOR->getFunctionById(FUNCTION_ID_IM), &mtest, NULL);
						m_im.calculateFunctions(eo);
						ComparisonResult cmp = m_im.compare(m_zero);
						if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) b = false;
					}
				}
			} else {
				b = false;
			}
			eo.expand = false;
			eo.approximation = APPROXIMATION_EXACT_VARIABLES;
			if(!b) {
				b = true;
				msolve.calculatesub(eo, eo, true);
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.expand = eo_pre.expand;
				msolve.factorize(eo, false, false, 0, false, true, NULL, m_undefined, false, false, 1);
				remove_nonzero_mul(msolve, u_var, eo);
				if(msolve.isZero()) {
				} else if(contains_undefined(msolve) || msolve.countTotalChildren(false) > 1000 || msolve.containsInterval(true, true, false, 1, true)) {
					b = false;
				} else {
					MathStructure mtest(mstruct);
					mtest.replace(v, u_var);
					mtest.calculatesub(eo, eo, true);
					if(contains_undefined(mtest, eo, true, u_var, mv)) {
						b = false;
					} else {
						Number nr_prec(1, 1, -(PRECISION + 10));
						nr_prec *= nr_intval.uncertainty();
						// search for values where derivative is non-zero
						b = find_interval_zeroes(msolve, malts, u_var, nr_intval, eo, 0, nr_prec);
					}
				}
				eo.expand = false;
				eo.approximation = APPROXIMATION_EXACT_VARIABLES;
			}
			eo.keep_zero_units = eo_pre.keep_zero_units;
		}
		CALCULATOR->endTemporaryStopMessages();
		CALCULATOR->beginTemporaryStopMessages();
		if(b) {
			malts.addChild(nr_intval.lowerEndPoint());
			malts.addChild(nr_intval.upperEndPoint());
			MathStructure mnew;
			for(size_t i = 0; i < malts.size(); i++) {
				MathStructure mlim(mstruct);
				if(!mmul.isOne()) malts[i] *= mmul;
				mlim.replace(v, malts[i]);
				mlim.calculatesub(eo, eo, true);
				vector<KnownVariable*> vars2 = vars;
				solve_intervals2(mlim, vars2, eo_pre);
				if(i == 0) {
					mnew = mlim;
				} else {
					MathStructure mlim1(mnew);
					if(!create_interval(mnew, mlim1, mlim)) {
						eo.approximation = APPROXIMATION_APPROXIMATE;
						eo.expand = eo_pre.expand;
						mlim.eval(eo);
						if(!create_interval(mnew, mlim1, mlim)) {
							mlim1.eval(eo);
							eo.expand = false;
							eo.approximation = APPROXIMATION_EXACT_VARIABLES;
							if(!create_interval(mnew, mlim1, mlim)) {
								b = false;
								break;
							}
						}
					}
				}
			}
			if(b) mstruct = mnew;
		}
		CALCULATOR->endTemporaryStopMessages(b);
		if(!b) {
			CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval potentially calculated wide."), NULL);
			mstruct.replace(v, v->get());
			mstruct.unformat(eo_pre);
			solve_intervals2(mstruct, vars, eo_pre);
		}
		u_var->destroy();
	}
}
KnownVariable *fix_find_interval_variable(MathStructure &mstruct) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		const MathStructure &m = ((KnownVariable*) mstruct.variable())->get();
		if(contains_interval_variable(m)) return (KnownVariable*) mstruct.variable();
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		KnownVariable *v = fix_find_interval_variable(mstruct[i]);
		if(v) return v;
	}
	return NULL;
}
KnownVariable *fix_find_interval_variable2(MathStructure &mstruct) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		const MathStructure &m = ((KnownVariable*) mstruct.variable())->get();
		if(m.isNumber()) return NULL;
		if(m.isMultiplication()) {
			bool b_intfound = false;;
			for(size_t i = 0; i < m.size(); i++) {
				if(m[i].containsInterval(true, false, false, 1)) {
					if(b_intfound || !m[i].isNumber()) return (KnownVariable*) mstruct.variable();
					b_intfound = true;
				}
			}
		} else if(m.containsInterval(true, false, false, 1)) {
			return (KnownVariable*) mstruct.variable();
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		KnownVariable *v = fix_find_interval_variable2(mstruct[i]);
		if(v) return v;
	}
	return NULL;
}
bool replace_intervals(MathStructure &m) {
	if(m.isNumber()) {
		int var_prec = 0;
		if(m.number().isInterval()) var_prec = m.number().precision(1);
		else if(CALCULATOR->usesIntervalArithmetic() && m.number().precision() >= 0) var_prec = m.number().precision();
		if(var_prec <= PRECISION + 10) {
			Variable *v = new KnownVariable("", format_and_print(m), m);
			m.set(v, true);
			v->destroy();
		}
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_intervals(m[i])) {
			m.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
void fix_interval_variable(KnownVariable *v, MathStructure &mvar) {
	mvar = v->get();
	replace_intervals(mvar);
}

void solve_intervals(MathStructure &mstruct, const EvaluationOptions &eo, const EvaluationOptions &feo) {
	bool b = false;
	while(true) {
		KnownVariable *v = fix_find_interval_variable(mstruct);
		if(!v) break;
		b = true;
		MathStructure mvar;
		fix_interval_variable(v, mvar);
		mstruct.replace(v, mvar);
	}
	while(true) {
		KnownVariable *v = fix_find_interval_variable2(mstruct);
		if(!v) break;
		b = true;
		MathStructure mvar;
		fix_interval_variable(v, mvar);
		mstruct.replace(v, mvar);
	}
	if(b) {
		mstruct.unformat(eo);
		EvaluationOptions eo2 = eo;
		eo2.expand = false;
		mstruct.calculatesub(eo2, feo, true);
	}
	vector<KnownVariable*> vars; vector<int> v_count; vector<int> v_prec;
	find_interval_variables(mstruct, vars, v_count, v_prec);
	for(size_t i = 0; i < v_count.size();) {
		if(v_count[i] < 2 || (feo.approximation == APPROXIMATION_EXACT && vars[i]->title() != "\b")) {
			v_count.erase(v_count.begin() + i);
			v_prec.erase(v_prec.begin() + i);
			vars.erase(vars.begin() + i);
		} else {
			i++;
		}
	}
	if(mstruct.isComparison()) {
		if(feo.test_comparisons || feo.isolate_x) {
			mstruct[0].subtract(mstruct[1]);
			solve_intervals2(mstruct[0], vars, eo);
			mstruct[1].clear(true);
		} else {
			solve_intervals2(mstruct[0], vars, eo);
			solve_intervals2(mstruct[1], vars, eo);
		}
		return;
	}
	solve_intervals2(mstruct, vars, eo);
}
// end functions for handling of interval variables using interval arithmetic


/*

	Functions for uncertainty calculation using variance formula

*/

void find_interval_create_var(const Number &nr, MathStructure &m, MathStructure &unc, MathStructure &unc2, KnownVariable **v, KnownVariable **v2) {
	if(nr.hasImaginaryPart() && nr.internalImaginary()->isInterval()) {
		if(nr.hasRealPart() && nr.isInterval(false)) {
			unc = nr.internalImaginary()->uncertainty();
			unc2 = nr.realPart().uncertainty();
			Number nmid(*nr.internalImaginary());
			nmid.intervalToMidValue();
			Number nmid2(nr.realPart());
			nmid2.intervalToMidValue();
			*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
			(*v)->setApproximate(false);
			*v2 = new KnownVariable("", string("(") + format_and_print(nmid2) + ")", nmid2);
			(*v2)->setApproximate(false);
			m.set(*v);
			m.multiply(nr_one_i);
			m.add(*v2);
			(*v)->destroy();
			(*v2)->destroy();
		} else {
			unc = nr.internalImaginary()->uncertainty();
			Number nmid(*nr.internalImaginary());
			nmid.intervalToMidValue();
			*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
			(*v)->setApproximate(false);
			m.set(*v);
			m.multiply(nr_one_i);
			(*v)->destroy();
		}
	} else {
		unc = nr.uncertainty();
		Number nmid(nr);
		nmid.intervalToMidValue();
		*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
		(*v)->setApproximate(false);
		m.set(*v);
		(*v)->destroy();
	}
}

KnownVariable *find_interval_replace_var(MathStructure &m, MathStructure &unc, MathStructure &unc2, KnownVariable **v2, const EvaluationOptions &eo, MathStructure *mnew, Variable **prev_v, bool &b_failed) {
	if(eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables && m.isVariable() && m.variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || m.variable()->title() == "\b")) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(!mvar.containsInterval(true, true, false, 1, true)) return NULL;
		if(mvar.isNumber()) {
			m.variable()->ref();
			*prev_v = m.variable();
			KnownVariable *v = NULL;
			find_interval_create_var(mvar.number(), m, unc, unc2, &v, v2);
			*mnew = m;
			return v;
		} else if(mvar.isMultiplication() && mvar[0].isNumber()) {
			if(mvar[0].number().isInterval(false)) {
				bool b = true;
				for(size_t i = 1; i < mvar.size(); i++) {
					if(mvar[i].containsInterval(true, true, false, 1, true)) {
						b = false;
						break;
					}
				}
				if(b) {
					m.variable()->ref();
					*prev_v = m.variable();
					KnownVariable *v = NULL;
					find_interval_create_var(mvar[0].number(), m, unc, unc2, &v, v2);
					for(size_t i = 1; i < mvar.size(); i++) {
						m.multiply(mvar[i], true);
					}
					*mnew = m;
					return v;
				}
			}
		} else if(mvar.isFunction() && mvar.function()->id() == FUNCTION_ID_INTERVAL && mvar.size() == 2 && !mvar[0].containsInterval(true, true, false, 1, true) && !mvar[1].containsInterval(true, true, false, 1, true)) {
			if(mvar[0].isAddition() && mvar[0].size() == 2 && mvar[1].isAddition() && mvar[1].size() == 2) {
				const MathStructure *mmid = NULL, *munc = NULL;
				if(mvar[0][0].equals(mvar[1][0])) {
					mmid = &mvar[0][0];
					if(mvar[0][1].isNegate() && mvar[0][1][0].equals(mvar[1][1])) munc = &mvar[1][1];
					if(mvar[1][1].isNegate() && mvar[1][1][0].equals(mvar[0][1])) munc = &mvar[0][1];
				} else if(mvar[0][1].equals(mvar[1][1])) {
					mmid = &mvar[0][1];
					if(mvar[0][0].isNegate() && mvar[0][0][0].equals(mvar[1][0])) munc = &mvar[1][0];
					if(mvar[1][0].isNegate() && mvar[1][0][0].equals(mvar[0][0])) munc = &mvar[0][0];
				} else if(mvar[0][0].equals(mvar[1][1])) {
					mmid = &mvar[0][0];
					if(mvar[0][1].isNegate() && mvar[0][1][0].equals(mvar[1][0])) munc = &mvar[1][0];
					if(mvar[1][0].isNegate() && mvar[1][0][0].equals(mvar[0][1])) munc = &mvar[0][1];
				} else if(mvar[0][1].equals(mvar[1][0])) {
					mmid = &mvar[0][0];
					if(mvar[0][0].isNegate() && mvar[0][0][0].equals(mvar[1][1])) munc = &mvar[1][1];
					if(mvar[1][1].isNegate() && mvar[1][1][0].equals(mvar[0][0])) munc = &mvar[0][0];
				}
				if(mmid && munc) {
					unc = *munc;
					MathStructure mmid2(*mmid);
					KnownVariable *v = new KnownVariable("", string("(") + format_and_print(*mmid) + ")", mmid2);
					m.set(v);
					v->destroy();
					return v;
				}
			}
			unc = mvar[1];
			unc -= mvar[0];
			unc *= nr_half;
			MathStructure mmid(mvar[0]);
			mmid += mvar[1];
			mmid *= nr_half;
			KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mmid) + ")", mmid);
			m.variable()->ref();
			*prev_v = m.variable();
			m.set(v);
			*mnew = m;
			v->destroy();
			return v;
		} else if(mvar.isFunction() && mvar.function()->id() == FUNCTION_ID_UNCERTAINTY && mvar.size() == 3 && mvar[2].isNumber() && !mvar[0].containsInterval(true, true, false, 1, true) && !mvar[1].containsInterval(true, true, false, 1, true)) {
			if(mvar[2].number().getBoolean()) {
				unc = mvar[1];
				unc *= mvar[0];
			} else {
				unc = mvar[1];
			}
			KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mvar[0]) + ")", mvar[0]);
			m.variable()->ref();
			*prev_v = m.variable();
			m.set(v);
			*mnew = m;
			v->destroy();
			return v;
		}
		b_failed = true;
	} else if(m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) {
		KnownVariable *v = NULL;
		find_interval_create_var(m.number(), m, unc, unc2, &v, v2);
		return v;
	} else if(m.isFunction() && m.function()->id() == FUNCTION_ID_INTERVAL && m.size() == 2 && !m[0].containsInterval(true, true, false, 1, true) && !m[1].containsInterval(true, true, false, 1, true)) {
		if(m[0].isAddition() && m[0].size() == 2 && m[1].isAddition() && m[1].size() == 2) {
			MathStructure *mmid = NULL, *munc = NULL;
			if(m[0][0].equals(m[1][0])) {
				mmid = &m[0][0];
				if(m[0][1].isNegate() && m[0][1][0].equals(m[1][1])) munc = &m[1][1];
				if(m[1][1].isNegate() && m[1][1][0].equals(m[0][1])) munc = &m[0][1];
			} else if(m[0][1].equals(m[1][1])) {
				mmid = &m[0][1];
				if(m[0][0].isNegate() && m[0][0][0].equals(m[1][0])) munc = &m[1][0];
				if(m[1][0].isNegate() && m[1][0][0].equals(m[0][0])) munc = &m[0][0];
			} else if(m[0][0].equals(m[1][1])) {
				mmid = &m[0][0];
				if(m[0][1].isNegate() && m[0][1][0].equals(m[1][0])) munc = &m[1][0];
				if(m[1][0].isNegate() && m[1][0][0].equals(m[0][1])) munc = &m[0][1];
			} else if(m[0][1].equals(m[1][0])) {
				mmid = &m[0][0];
				if(m[0][0].isNegate() && m[0][0][0].equals(m[1][1])) munc = &m[1][1];
				if(m[1][1].isNegate() && m[1][1][0].equals(m[0][0])) munc = &m[0][0];
			}
			if(mmid && munc) {
				unc = *munc;
				KnownVariable *v = new KnownVariable("", string("(") + format_and_print(*mmid) + ")", *mmid);
				m.set(v);
				v->destroy();
				return v;
			}
		}
		unc = m[1];
		unc -= m[0];
		unc *= nr_half;
		MathStructure mmid(m[0]);
		mmid += m[1];
		mmid *= nr_half;
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mmid) + ")", mmid);
		m.set(v);
		v->destroy();
		return v;
	} else if(m.isFunction() && m.function()->id() == FUNCTION_ID_UNCERTAINTY && m.size() == 3 && m[2].isNumber() && !m[0].containsInterval(true, true, false, 1, true) && !m[1].containsInterval(true, true, false, 1, true)) {
		if(m[2].number().getBoolean()) {
			unc = m[1];
			unc *= m[0];
		} else {
			unc = m[1];
		}
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(m[0]) + ")", m[0]);
		m.set(v);
		v->destroy();
		return v;
	}
	for(size_t i = 0; i < m.size(); i++) {
		KnownVariable *v = find_interval_replace_var(m[i], unc, unc2, v2, eo, mnew, prev_v, b_failed);
		if(b_failed) return NULL;
		if(v) return v;
	}
	return NULL;
}

bool find_interval_replace_var_nr(MathStructure &m) {
	if((m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) || (m.isFunction() && m.function()->id() == FUNCTION_ID_INTERVAL && m.size() == 2) || (m.isFunction() && m.function()->id() == FUNCTION_ID_UNCERTAINTY && m.size() == 3)) {
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(m) + ")", m);
		m.set(v);
		v->destroy();
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(find_interval_replace_var_nr(m[i])) b = true;
	}
	return b;
}


bool replace_variables_with_interval(MathStructure &m, const EvaluationOptions &eo, bool in_nounit = false, bool only_argument_vars = false) {
	if(m.isVariable() && m.variable()->isKnown() && (!only_argument_vars || m.variable()->title() == "\b")) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(!mvar.containsInterval(true, true, false, 1, true)) return false;
		if(mvar.isNumber()) {
			return false;
		} else if(mvar.isMultiplication() && mvar[0].isNumber()) {
			if(mvar[0].number().isInterval(false)) {
				bool b = true;
				for(size_t i = 1; i < mvar.size(); i++) {
					if(mvar[i].containsInterval(true, true, false, 1, true)) {
						b = false;
						break;
					}
				}
				if(b) return false;
			}
		}
		m.set(mvar, true);
		if(in_nounit) m.removeType(STRUCT_UNIT);
		else m.unformat(eo);
		return true;
	}
	bool b = false;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS && m.size() == 1) {
		b = replace_variables_with_interval(m[0], eo, true, only_argument_vars);
		if(b && m[0].containsType(STRUCT_UNIT, false, true, true) == 0) {
			m.setToChild(1, true);
		}
		return b;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_variables_with_interval(m[i], eo, in_nounit, only_argument_vars)) b = true;
	}
	return b;
}

bool contains_diff_for(const MathStructure &m, const MathStructure &x_var) {
	if(m.isFunction() && m.function() && m.function()->id() == FUNCTION_ID_DIFFERENTIATE && m.size() >= 2 && m[1] == x_var) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_diff_for(m[i], x_var)) return true;
	}
	return false;
}

MathStructure calculate_uncertainty(MathStructure &m, const EvaluationOptions &eo, bool &b_failed) {
	vector<KnownVariable*> vars;
	vector<MathStructure> uncs;
	MathStructure unc, unc2;
	if(eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) replace_variables_with_interval(m, eo, false, eo.approximation == APPROXIMATION_EXACT);
	while(true) {
		Variable *prev_v = NULL;
		MathStructure mnew;
		KnownVariable *v2 = NULL;
		KnownVariable *v = find_interval_replace_var(m, unc, unc2, &v2, eo, &mnew, &prev_v, b_failed);
		if(!v) break;
		if(!mnew.isZero()) {
			m.replace(prev_v, mnew);
			prev_v->unref();
		}
		vars.push_back(v);
		uncs.push_back(unc);
		if(v2) {
			vars.push_back(v2);
			uncs.push_back(unc2);
		}
	}
	m.unformat(eo);
	if(eo.sync_units && eo.approximation != APPROXIMATION_EXACT) sync_approximate_units(m, eo, &vars, &uncs, true);
	if(b_failed || vars.empty()) return m_zero;
	MathStructure munc;
	UnknownVariable *uv = new UnknownVariable("", "x");
	MathStructure muv(uv);
	MathStructure *munc_i = NULL;
	for(size_t i = 0; i < vars.size(); i++) {
		if(!vars[i]->get().representsNonComplex(true)) {
			b_failed = true; return m_zero;
		}
		MathStructure *mdiff = new MathStructure(m);
		uv->setInterval(vars[i]->get());
		mdiff->replace(vars[i], muv);
		if(!mdiff->differentiate(muv, eo) || contains_diff_for(*mdiff, muv) || CALCULATOR->aborted()) {
			b_failed = true;
			return m_zero;
		}
		mdiff->replace(muv, vars[i]);
		if(!mdiff->representsNonComplex(true)) {
			MathStructure *mdiff_i = new MathStructure(*mdiff);
			mdiff->transformById(FUNCTION_ID_RE);
			mdiff_i->transformById(FUNCTION_ID_IM);
			mdiff_i->raise(nr_two);
			mdiff_i->multiply(uncs[i]);
			mdiff_i->last().raise(nr_two);
			if(!munc_i) {munc_i = mdiff_i;}
			else munc_i->add_nocopy(mdiff_i, true);
		}
		mdiff->raise(nr_two);
		mdiff->multiply(uncs[i]);
		mdiff->last().raise(nr_two);
		if(munc.isZero()) {munc.set_nocopy(*mdiff); mdiff->unref();}
		else munc.add_nocopy(mdiff, true);
	}
	uv->destroy();
	munc.raise(nr_half);
	if(munc_i) {
		munc_i->raise(nr_half);
		munc_i->multiply(nr_one_i);
		munc.add_nocopy(munc_i);
	}
	return munc;
}

Variable *find_interval_replace_var_comp(MathStructure &m, const EvaluationOptions &eo, Variable **v) {
	if(eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables && m.isVariable() && m.variable()->isKnown() && ((KnownVariable*) m.variable())->get().containsInterval(true, true, false, 1, true)) {
		UnknownVariable *uv = new UnknownVariable("", format_and_print(m));
		uv->setInterval(m);
		*v = m.variable();
		m.set(uv, true);
		return uv;
	} else if((m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) || (m.isFunction() && m.function()->id() == FUNCTION_ID_INTERVAL && m.size() == 2) || (m.isFunction() && m.function()->id() == FUNCTION_ID_UNCERTAINTY && m.size() == 3)) {
		Variable *uv = NULL;
		if(eo.approximation == APPROXIMATION_EXACT || eo.approximation == APPROXIMATION_EXACT_VARIABLES) {
			uv = new KnownVariable("", string("(") + format_and_print(m) + ")", m);
		} else {
			uv = new UnknownVariable("", string("(") + format_and_print(m) + ")");
			((UnknownVariable*) uv)->setInterval(m);
		}
		*v = NULL;
		m.set(uv, true);
		return uv;
	}
	for(size_t i = 0; i < m.size(); i++) {
		Variable *uv = find_interval_replace_var_comp(m[i], eo, v);
		if(uv) return uv;
	}
	return NULL;
}

bool eval_comparison_sides(MathStructure &m, const EvaluationOptions &eo) {
	if(m.isComparison()) {
		MathStructure mbak(m);
		if(!m[0].isUnknown()) {
			bool ret = true;
			CALCULATOR->beginTemporaryStopMessages();
			m[0].eval(eo);
			if(m[0].containsFunctionId(FUNCTION_ID_UNCERTAINTY) && !mbak[0].containsFunctionId(FUNCTION_ID_UNCERTAINTY)) {
				CALCULATOR->endTemporaryStopMessages();
				m[0] = mbak[0];
				ret = false;
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
			}
			CALCULATOR->beginTemporaryStopMessages();
			m[1].eval(eo);
			if(m[1].containsFunctionId(FUNCTION_ID_UNCERTAINTY) && !mbak[1].containsFunctionId(FUNCTION_ID_UNCERTAINTY)) {
				CALCULATOR->endTemporaryStopMessages();
				m[1] = mbak[1];
				ret = false;
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
			}
			if(ret && !m.containsUnknowns()) {
				m.calculatesub(eo, eo, false);
				return true;
			}
			return false;
		} else {
			m[1].eval(eo);
			m.calculatesub(eo, eo, false);
			return true;
		}
	} else if(m.containsType(STRUCT_COMPARISON)) {
		bool ret = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(!eval_comparison_sides(m[i], eo)) ret = false;
		}
		m.childrenUpdated();
		m.calculatesub(eo, eo, false);
		return ret;
	} else {
		m.eval(eo);
	}
	return true;
}

bool separate_unit_vars(MathStructure &m, const EvaluationOptions &eo, bool only_approximate, bool dry_run) {
	if(m.isVariable() && m.variable()->isKnown()) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(mvar.isMultiplication()) {
			bool b = false;
			for(size_t i = 0; i < mvar.size(); i++) {
				if(is_unit_multiexp(mvar[i])) {
					if(!b) b = !only_approximate || contains_approximate_relation_to_base(mvar[i], true);
				} else if(mvar[i].containsType(STRUCT_UNIT, false, true, true) != 0) {
					b = false;
					break;
				}
			}
			if(!b) return false;
			if(dry_run) return true;
			m.transformById(FUNCTION_ID_STRIP_UNITS);
			for(size_t i = 0; i < mvar.size(); i++) {
				if(is_unit_multiexp(mvar[i])) {
					m.multiply(mvar[i], i);
				}
			}
			m.unformat(eo);
			return true;
		}
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(separate_unit_vars(m[i], eo, only_approximate, dry_run)) {
			b = true;
			if(dry_run) return true;
		}
	}
	return b;
}
// end functions for uncertainty calculation using variance formula

// Functions for handling of function arguments with intervals replaced by temporary variables
int contains_interval_var(const MathStructure &m, bool structural_only, bool check_variables, bool check_functions, int ignore_high_precision_interval, bool include_interval_function);
int contains_function_interval(const MathStructure &m, bool structural_only = true, bool check_variables = false, bool check_functions = false, int ignore_high_precision_interval = 0, bool include_interval_function = false) {
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_function_interval(m[i], structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function)) return true;
	}
	if(m.isVariable() && m.variable()->isKnown() && m.variable()->title() == "\b") {
		if(ignore_high_precision_interval == 0) return true;
		return contains_interval_var(((KnownVariable*) m.variable())->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
	}
	return false;
}
bool replace_function_vars(MathStructure &m) {
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_function_vars(m[i])) return true;
	}
	if(m.isVariable() && m.variable()->isKnown() && m.variable()->title() == "\b") {
		m.set(((KnownVariable*) m.variable())->get(), true);
	}
	return false;
}

// Simplify expressions by combining sin() and cos()
bool simplify_functions(MathStructure &mstruct, const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_var) {
	if(!mstruct.isAddition()) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(simplify_functions(mstruct[i], eo, feo, x_var)) {b = true; mstruct.childUpdated(i + 1);}
		}
		return b;
	}
	if(mstruct.containsFunctionId(FUNCTION_ID_SIN, false, false, false) > 0 && mstruct.containsFunctionId(FUNCTION_ID_COS, false, false, false)) {
		if(x_var.isUndefined()) {
			// a*(sin(x)+cos(x))=a*sqrt(2)*sin(x+pi/4)
			bool b_ret = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isFunction() && mstruct[i].size() == 1 && mstruct[i].function()->id() == FUNCTION_ID_SIN) {
					for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
						if(i != i2 && mstruct[i2].isFunction() && mstruct[i2].size() == 1 && mstruct[i2].function()->id() == FUNCTION_ID_COS && mstruct[i][0] == mstruct[i2][0]) {
							MathStructure madd(CALCULATOR->getVariableById(VARIABLE_ID_PI));
							madd /= Number(4, 1);
							madd *= CALCULATOR->getRadUnit();
							madd.calculatesub(eo, feo, true);
							mstruct[i][0].calculateAdd(madd, eo);
							mstruct[i].childUpdated(1);
							mstruct.childUpdated(i + 1);
							MathStructure mmul(nr_two);
							mmul.calculateRaise(nr_half, eo);
							mstruct[i].calculateMultiply(mmul, eo);
							mstruct.delChild(i2 + 1);
							b_ret = true;
							break;
						}
					}
				} else if(mstruct[i].isMultiplication()) {
					for(size_t i3 = 0; i3 < mstruct[i].size(); i3++) {
						if(mstruct[i][i3].isFunction() && mstruct[i][i3].size() == 1 && mstruct[i][i3].function()->id() == FUNCTION_ID_SIN) {
							mstruct[i][i3].setFunctionId(FUNCTION_ID_COS);
							bool b = false;
							for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
								if(i != i2 && mstruct[i2] == mstruct[i]) {
									MathStructure madd(CALCULATOR->getVariableById(VARIABLE_ID_PI));
									madd /= Number(4, 1);
									madd *= CALCULATOR->getRadUnit();
									madd.calculatesub(eo, feo, true);
									mstruct[i][i3].setFunctionId(FUNCTION_ID_SIN);
									mstruct[i][i3][0].calculateAdd(madd, eo);
									mstruct[i][i3].childUpdated(1);
									mstruct[i].childUpdated(i3 + 1);
									mstruct.childUpdated(i + 1);
									MathStructure mmul(nr_two);
									mmul.calculateRaise(nr_half, eo);
									mstruct[i].calculateMultiply(mmul, eo);
									mstruct.delChild(i2 + 1);
									b = true;
									break;
								}
							}
							if(b) {
								b_ret = true;
								break;
							} else {
								mstruct[i][i3].setFunctionId(FUNCTION_ID_SIN);
							}
						}
					}
				}
			}
			if(mstruct.size() == 1) mstruct.setToChild(1, true);
			return b_ret;
		} else {
			// a*sin(x)+b*cos(x)=a*sqrt((b/a)^2+1)*sin(x+atan(b/a))
			MathStructure *marg = NULL;
			bool b_cos = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isFunction() && mstruct[i].size() == 1 && (mstruct[i].function()->id() == FUNCTION_ID_SIN || mstruct[i].function()->id() == FUNCTION_ID_COS) && mstruct[i][0].contains(x_var)) {
					marg = &mstruct[i][0];
					b_cos = mstruct[i].function()->id() == FUNCTION_ID_COS;
				} else if(mstruct[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
						if(!marg && mstruct[i][i2].isFunction() && mstruct[i][i2].size() == 1 && (mstruct[i][i2].function()->id() == FUNCTION_ID_SIN || mstruct[i][i2].function()->id() == FUNCTION_ID_COS) && mstruct[i][i2][0].contains(x_var)) {
							marg = &mstruct[i][i2][0];
							b_cos = mstruct[i][i2].function()->id() == FUNCTION_ID_COS;
						} else if(mstruct[i][i2].contains(x_var)) {
							marg = NULL;
							break;
						}
					}
				}
				if(marg) {
					bool b = false;
					for(size_t i3 = i + 1; i3 < mstruct.size(); i3++) {
						if(mstruct[i3].isFunction() && mstruct[i3].size() == 1 && mstruct[i3].function()->id() == (b_cos ? FUNCTION_ID_SIN : FUNCTION_ID_COS) && mstruct[i3][0] == *marg) {
							b = true;
						} else if(mstruct[i3].isMultiplication()) {
							bool b2 = false;
							for(size_t i2 = 0; i2 < mstruct[i3].size(); i2++) {
								if(!b2 && mstruct[i3][i2].isFunction() && mstruct[i3][i2].size() == 1 && (mstruct[i3][i2].function()->id() == FUNCTION_ID_SIN || mstruct[i3][i2].function()->id() == FUNCTION_ID_COS) && mstruct[i3][i2][0] == *marg) {
									if((mstruct[i3][i2].function()->id() == FUNCTION_ID_SIN) == b_cos) b = true;
									b2 = true;
								} else if(mstruct[i3][i2].contains(x_var)) {
									marg = NULL;
									break;
								}
							}
							if(!marg) break;
						}
					}
					if(!b) marg = NULL;
				}
				if(marg) {
					marg->ref();
					MathStructure m_a, m_b;
					for(size_t i3 = i; i3 < mstruct.size();) {
						bool b = false;
						if(mstruct[i3].isFunction() && mstruct[i3].size() == 1 && mstruct[i3].function()->id() == FUNCTION_ID_SIN && mstruct[i3][0] == *marg) {
							if(m_a.isZero()) m_a = m_one;
							else m_a.add(m_one, true);
							b = true;
						} else if(mstruct[i3].isFunction() && mstruct[i3].size() == 1 && mstruct[i3].function()->id() == FUNCTION_ID_COS && mstruct[i3][0] == *marg) {
							if(m_b.isZero()) m_b = m_one;
							else m_b.add(m_one, true);
							b = true;
						} else if(mstruct[i3].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct[i3].size(); i2++) {
								if(mstruct[i3][i2].isFunction() && mstruct[i3][i2].size() == 1 && mstruct[i3][i2].function()->id() == FUNCTION_ID_SIN && mstruct[i3][i2][0] == *marg) {
									mstruct[i3].delChild(i2 + 1, true);
									if(m_a.isZero()) m_a.set_nocopy(mstruct[i3]);
									else {mstruct[i3].ref(); m_a.add_nocopy(&mstruct[i3], true);}
									b = true;
									break;
								} else if(mstruct[i3][i2].isFunction() && mstruct[i3][i2].size() == 1 && mstruct[i3][i2].function()->id() == FUNCTION_ID_COS && mstruct[i3][i2][0] == *marg) {
									mstruct[i3].delChild(i2 + 1, true);
									if(m_b.isZero()) m_b.set_nocopy(mstruct[i3]);
									else {mstruct[i3].ref(); m_b.add_nocopy(&mstruct[i3], true);}
									b = true;
									break;
								}
							}
						}
						if(b) {
							mstruct.delChild(i3 + 1);
						} else {
							i3++;
						}
					}
					MathStructure *m_sin = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), NULL);
					m_sin->addChild_nocopy(marg);
					m_b.calculateDivide(m_a, eo);
					MathStructure *m_atan = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &m_b, NULL);
					if(m_atan->calculateFunctions(feo)) m_atan->calculatesub(eo, feo, true);
					if(HAS_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) m_atan->calculateMultiply(CALCULATOR->getRadUnit(), eo);
					(*m_sin)[0].add_nocopy(m_atan);
					(*m_sin)[0].calculateAddLast(eo);
					m_sin->childUpdated(1);
					m_b.calculateRaise(nr_two, eo);
					m_b.calculateAdd(m_one, eo);
					m_b.calculateRaise(nr_half, eo);
					m_sin->calculateMultiply(m_b, eo);
					m_sin->calculateMultiply(m_a, eo);
					if(mstruct.size() == 0) {mstruct.set_nocopy(*m_sin); m_sin->unref();}
					else mstruct.insertChild_nocopy(m_sin, i + 1);
					simplify_functions(mstruct, eo, feo, x_var);
					return true;
				}
			}
		}
	}
	return false;
}

// Simplify expressions by combining logarithms
bool simplify_ln(MathStructure &mstruct) {
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(simplify_ln(mstruct[i])) b_ret = true;
	}
	if(mstruct.isAddition()) {
		size_t i_ln = (size_t) -1, i_ln_m = (size_t) -1;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isFunction() && mstruct[i].function()->id() == FUNCTION_ID_LOG && mstruct[i].size() == 1 && mstruct[i][0].isNumber() && mstruct[i][0].number().isReal() && mstruct[i][0].number().isNonZero()) {
				if(i_ln == (size_t) -1) {
					i_ln = i;
				} else {
					bool b = true;
					if(mstruct[i_ln].isMultiplication()) {
						if(mstruct[i_ln][1][0].number().raise(mstruct[i_ln][0].number(), true)) {mstruct[i_ln].setToChild(2, true); b_ret = true;}
						else b = false;
					}
					if(b && (!mstruct[i][0].number().isNegative() || !mstruct[i_ln][0].number().isNegative()) && mstruct[i_ln][0].number().multiply(mstruct[i][0].number())) {
						mstruct.delChild(i + 1);
						i--;
						b_ret = true;
					}
				}
			} else if(mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1].isFunction() && mstruct[i][1].function()->id() == FUNCTION_ID_LOG && mstruct[i][1].size() == 1 && mstruct[i][1][0].isNumber() && mstruct[i][1][0].number().isReal() && mstruct[i][1][0].number().isNonZero() && mstruct[i][0].isInteger() && mstruct[i][0].number().isLessThan(1000) && mstruct[i][0].number().isGreaterThan(-1000)) {
				if(mstruct[i][0].number().isPositive()) {
					if(i_ln == (size_t) -1) {
						i_ln = i;
					} else {
						bool b = true;
						if(mstruct[i_ln].isMultiplication()) {
							if(mstruct[i_ln][1][0].number().raise(mstruct[i_ln][0].number())) {mstruct[i_ln].setToChild(2, true); b_ret = true;}
							else b = false;
						}
						if(b && mstruct[i][1][0].number().raise(mstruct[i][0].number(), true)) {
							if((!mstruct[i][1][0].number().isNegative() || !mstruct[i_ln][0].number().isNegative()) && mstruct[i_ln][0].number().multiply(mstruct[i][1][0].number())) {
								mstruct.delChild(i + 1);
								i--;
							} else {
								mstruct[i].setToChild(1, true);
							}
							b_ret = true;
						}
					}
				} else if(mstruct[i][0].number().isNegative()) {
					if(i_ln_m == (size_t) -1) {
						i_ln_m = i;
					} else {
						bool b = mstruct[i_ln_m][0].number().isMinusOne();
						if(!b && mstruct[i_ln_m][1][0].number().raise(-mstruct[i_ln_m][0].number())) {mstruct[i_ln_m][0].set(m_minus_one, true); b_ret = true; b = true;}
						bool b_m1 = b && mstruct[i][0].number().isMinusOne();
						if(b && (b_m1 || mstruct[i][1][0].number().raise(-mstruct[i][0].number(), true))) {
							if((!mstruct[i][1][0].number().isNegative() || !mstruct[i_ln_m][1][0].number().isNegative()) && mstruct[i_ln_m][1][0].number().multiply(mstruct[i][1][0].number())) {
								mstruct.delChild(i + 1);
								b_ret = true;
								i--;
							} else if(!b_m1) b_ret = true;
						}
					}
				}
			}
		}
		if(mstruct.size() == 1) mstruct.setToChild(1, true);
	}
	return b_ret;
}

bool simplify_roots(MathStructure &mstruct, const EvaluationOptions &eo) {
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(simplify_roots(mstruct[i], eo)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(mstruct.isMultiplication() && mstruct.size() >= 2 && mstruct[0].isNumber() && mstruct[0].number().isRational()) {
		for(size_t i = 1; i < mstruct.size(); i++) {
			if(mstruct[i].isPower() && mstruct[i][1].isNumber() && !mstruct[i][1].number().includesInfinity() && mstruct[i][0].isNumber() && mstruct[i][0].number().isRational() && !mstruct[i][0].number().isZero()) {
				if(mstruct[0].number().denominator() == mstruct[i][0].number().numerator() && mstruct[0].number().numerator() == mstruct[i][0].number().denominator()) {
					// (n/m)^a*m/n=(n/m)^(a-1)
					mstruct[i][1].number()--;
					mstruct.childUpdated(i + 1);
					mstruct.delChild(1, true);
					b_ret = true;
					break;
				} else if(mstruct[i][1].number().isNegative() && mstruct[0].number().isIntegerDivisible(mstruct[i][0].number())) {
					if(mstruct[0].number().divide(mstruct[i][0].number())) {
						mstruct[0].numberUpdated();
						mstruct.childUpdated(1);
						mstruct[i][1].number()++;
						b_ret = true;
						if(mstruct[0].isOne()) {mstruct.delChild(1); break;}
					}
				} else if(mstruct[i][1].number().isPositive() && !mstruct[0].number().isInteger() && mstruct[0].number().denominator().isIntegerDivisible(mstruct[i][0].number())) {
					if(mstruct[0].number().multiply(mstruct[i][0].number())) {
						mstruct[0].numberUpdated();
						mstruct.childUpdated(1);
						mstruct[i][1].number()--;
						b_ret = true;
						if(mstruct[0].isOne()) {mstruct.delChild(1, true); break;}
					}
				}
			}
		}
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower()) {
				for(size_t i2 = i + 1; i2 < mstruct.size();) {
					if(mstruct[i2].isPower() && mstruct[i][0] == mstruct[i2][0]) {
						if(!eo.allow_complex && !mstruct[i][0].representsNonNegative(true) && (!mstruct[i][1].representsInteger() || !mstruct[i2][1].representsInteger())) {
							break;
						}
						mstruct[i][1].add(mstruct[i2][1], true);
						mstruct[i][1].calculateAddLast(eo);
						mstruct.delChild(i2 + 1);
						if(mstruct.size() == 1) {
							mstruct.setToChild(1, true);
							return true;
						}
						b_ret = true;
					} else {
						i2++;
					}
				}
			}
		}
	}
	return b_ret;
}

// Convert complex numbers from rectangular form
bool MathStructure::complexToExponentialForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		eo2.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
		MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		marg *= nr_one_i;
		marg.eval(eo2);
		set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
		raise(marg);
		mabs.eval(eo2);
		if(!mabs.isOne()) multiply(mabs);
		evalSort(false);
		return true;
	} else if(representsReal(true)) {
		return false;
	} else {
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		marg *= nr_one_i;
		CALCULATOR->beginTemporaryStopMessages();
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		eo2.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
		marg.eval(eo2);
		if(!marg.isFunction() || marg.function()->id() != FUNCTION_ID_ARG) {
			CALCULATOR->endTemporaryStopMessages(true);
			MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
			set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
			raise(marg);
			mabs.eval(eo2);
			if(!mabs.isOne()) multiply(mabs);
			evalSort(false);
			return true;
		}
		CALCULATOR->endTemporaryStopMessages();
	}
	if(m_type == STRUCT_POWER) return false;
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToExponentialForm(eo)) {b = true; CHILD_UPDATED(i);}
	}
	return b;
}
bool MathStructure::complexToPolarForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		mabs.eval(eo2);
		marg.eval(eo2);
		Unit *u = default_angle_unit(eo);
		if(u) marg.multiply(u, true);
		set(marg, true);
		transformById(FUNCTION_ID_SIN);
		multiply(nr_one_i);
		add_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &marg, NULL));
		if(!mabs.isOne()) multiply(mabs);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart()) {
		MathStructure marg(CHILD(1).number().imaginaryPart());
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {marg.calculateMultiply(Number(180, 1), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->getDegUnit(), true); break;}
			case ANGLE_UNIT_GRADIANS: {marg.calculateMultiply(Number(200, 1), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->getGraUnit(), true); break;}
			case ANGLE_UNIT_RADIANS: {marg.multiply(CALCULATOR->getRadUnit(), true); break;}
			case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {marg.calculateMultiply(angle_units_in_turn(eo2, 1, 2), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->customAngleUnit(), true);} break;}
			default: {break;}
		}
		set(marg, true);
		transformById(FUNCTION_ID_SIN);
		multiply(nr_one_i);
		add_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &marg, NULL));
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_MULTIPLICATION && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isVariable() && CHILD(1)[0].variable()->id() == VARIABLE_ID_E && CHILD(1)[1].isNumber() && CHILD(1)[1].number().hasImaginaryPart() && !CHILD(1)[1].number().hasRealPart() && CHILD(0).isNumber() && !CHILD(0).number().hasImaginaryPart()) {
		MathStructure marg(CHILD(1)[1].number().imaginaryPart());
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {marg.calculateMultiply(Number(180, 1), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->getDegUnit(), true); break;}
			case ANGLE_UNIT_GRADIANS: {marg.calculateMultiply(Number(200, 1), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->getGraUnit(), true); break;}
			case ANGLE_UNIT_RADIANS: {marg.multiply(CALCULATOR->getRadUnit(), true); break;}
			case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {marg.calculateMultiply(angle_units_in_turn(eo2, 1, 2), eo2); marg.calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); marg.multiply(CALCULATOR->customAngleUnit(), true);} break;}
			default: {break;}
		}
		CHILD(1).set(marg, true);
		CHILD(1).transformById(FUNCTION_ID_SIN);
		CHILD(1).multiply(nr_one_i);
		CHILD(1).add_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &marg, NULL));
		CHILD_UPDATED(1)
		evalSort(true);
		return true;
	} else if(representsReal(true)) {
		return false;
	} else {
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		CALCULATOR->beginTemporaryStopMessages();
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		marg.eval(eo2);
		if(!marg.isFunction() || marg.function()->id() != FUNCTION_ID_ARG) {
			CALCULATOR->endTemporaryStopMessages(true);
			MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
			mabs.eval(eo2);
			Unit *u = default_angle_unit(eo);
			if(u) marg.multiply(u);
			set(marg, true);
			transformById(FUNCTION_ID_SIN);
			multiply(nr_one_i);
			add_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &marg, NULL));
			if(!mabs.isOne()) multiply(mabs);
			evalSort(true);
			return true;
		}
		CALCULATOR->endTemporaryStopMessages();
	}
	if(m_type == STRUCT_POWER || m_type == STRUCT_FUNCTION) return false;
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToPolarForm(eo)) {b = true; CHILD_UPDATED(i);}
	}
	return b;
}
bool MathStructure::complexToCisForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		mabs.eval(eo2);
		marg.eval(eo2);
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {marg.multiply(CALCULATOR->getDegUnit(), true); break;}
			case ANGLE_UNIT_GRADIANS: {marg.multiply(CALCULATOR->getGraUnit(), true); break;}
			case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {marg.multiply(CALCULATOR->customAngleUnit(), true);} break;}
			default: {break;}
		}
		set(marg, true);
		transformById(FUNCTION_ID_CIS);
		multiply(mabs);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart()) {
		set(CHILD(1).number().imaginaryPart(), true);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {calculateMultiply(Number(180, 1), eo2); calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); multiply(CALCULATOR->getDegUnit(), true); break;}
			case ANGLE_UNIT_GRADIANS: {calculateMultiply(Number(200, 1), eo2); calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); multiply(CALCULATOR->getGraUnit(), true); break;}
			case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {calculateMultiply(angle_units_in_turn(eo2, 1, 2), eo2); calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); multiply(CALCULATOR->customAngleUnit(), true);} break;}
			default: {break;}
		}
		transformById(FUNCTION_ID_CIS);
		multiply(m_one);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_MULTIPLICATION && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isVariable() && CHILD(1)[0].variable()->id() == VARIABLE_ID_E && CHILD(1)[1].isNumber() && CHILD(1)[1].number().hasImaginaryPart() && !CHILD(1)[1].number().hasRealPart() && CHILD(0).isNumber() && !CHILD(0).number().hasImaginaryPart()) {
		CHILD(1).set(CHILD(1)[1].number().imaginaryPart(), true);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {CHILD(1).calculateMultiply(Number(180, 1), eo2); CHILD(1).calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); CHILD(1).multiply(CALCULATOR->getDegUnit(), true); break;}
			case ANGLE_UNIT_GRADIANS: {CHILD(1).calculateMultiply(Number(200, 1), eo2); CHILD(1).calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); CHILD(1).multiply(CALCULATOR->getGraUnit(), true); break;}
			case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {CHILD(1).calculateMultiply(angle_units_in_turn(eo2, 1, 2), eo2); CHILD(1).calculateDivide(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo2); CHILD(1).multiply(CALCULATOR->customAngleUnit(), true);} break;}
			default: {break;}
		}
		CHILD(1).transformById(FUNCTION_ID_CIS);
		CHILD_UPDATED(1)
		return true;
	} else if(representsReal(true)) {
		return false;
	} else {
		MathStructure marg(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), this, NULL);
		CALCULATOR->beginTemporaryStopMessages();
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		marg.eval(eo2);
		if(!marg.isFunction() || marg.function()->id() != FUNCTION_ID_ARG) {
			CALCULATOR->endTemporaryStopMessages(true);
			MathStructure mabs(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), this, NULL);
			mabs.eval(eo2);
			switch(eo2.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {marg.multiply(CALCULATOR->getDegUnit(), true); break;}
				case ANGLE_UNIT_GRADIANS: {marg.multiply(CALCULATOR->getGraUnit(), true); break;}
				case ANGLE_UNIT_CUSTOM: {if(CALCULATOR->customAngleUnit()) {marg.multiply(CALCULATOR->customAngleUnit(), true);} break;}
				default: {break;}
			}
			set(marg, true);
			transformById(FUNCTION_ID_CIS);
			multiply(mabs);
			evalSort(true);
			return true;
		}
		CALCULATOR->endTemporaryStopMessages();
	}
	bool b = false;
	size_t i_cis = SIZE;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToCisForm(eo)) {
			b = true; CHILD_UPDATED(i);
			if(i_cis == SIZE) i_cis = i;
			else i_cis = SIZE + 1;
		}
	}
	if(b && SIZE >= 2 && isMultiplication() && i_cis < SIZE && CHILD(i_cis).isMultiplication() && CHILD(i_cis).size() == 2 && CHILD(i_cis).last().isFunction() && CHILD(i_cis).last().function()->id() == FUNCTION_ID_CIS && CHILD(i_cis)[0].isOne()) {
		CHILD(i_cis).setToChild(2, true);
		if(SIZE > 2) {
			MathStructure *m_cis = &CHILD(i_cis);
			m_cis->ref();
			ERASE(i_cis)
			multiply_nocopy(m_cis);
		}
	}
	return b;
}

bool convert_to_default_angle_unit(MathStructure &m, const EvaluationOptions &eo) {
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(convert_to_default_angle_unit(m[i], eo)) b = true;
		if(m.isFunction() && m.function()->getArgumentDefinition(i + 1) && m.function()->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_ANGLE) {
			Unit *u = NULL;
			if(eo.parse_options.angle_unit == ANGLE_UNIT_DEGREES) u = CALCULATOR->getDegUnit();
			else if(eo.parse_options.angle_unit == ANGLE_UNIT_GRADIANS) u = CALCULATOR->getGraUnit();
			else if(eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) u = CALCULATOR->customAngleUnit();
			if(u && m[i].contains(CALCULATOR->getRadUnit(), false, false, false)) {
				m[i].divide(u);
				m[i].multiply(u);
				EvaluationOptions eo2 = eo;
				if(eo.approximation == APPROXIMATION_TRY_EXACT) eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo2.calculate_functions = false;
				eo2.sync_units = true;
				m[i].calculatesub(eo2, eo2, true);
				b = true;
			}
		}
	}
	return b;
}

// Remove zero unit values if remaining value is not zero (e.g. 5 s + 0 m = 5 s)
bool remove_add_zero_unit(MathStructure &m) {
	if(m.isAddition() && m.size() > 1) {
		bool b = false, b2 = false;
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isMultiplication() && m[i].size() > 1 && m[i][0].isZero() && !m[i].isApproximate()) {
				b = true;
			} else {
				b2 = true;
			}
			if(b && b2) break;
		}
		if(!b || !b2) return false;
		b = false;
		for(size_t i = 0; i < m.size();) {
			b2 = false;
			if(m[i].isMultiplication() && m[i].size() > 1 && m[i][0].isZero() && !m[i].isApproximate()) {
				b2 = true;
				for(size_t i2 = 1; i2 < m[i].size(); i2++) {
					if(!m[i][i2].isUnit_exp() || (m[i][i2].isPower() && m[i][i2][0].unit()->hasNonlinearRelationToBase()) || (m[i][i2].isUnit() && m[i][i2].unit()->hasNonlinearRelationToBase())) {
						b2 = false;
						break;
					}
				}
				if(b2) {
					b = true;
					m.delChild(i + 1);
					if(m.size() == 1) {
						m.setToChild(1, true);
						break;
					}
				}
			}
			if(!b2) i++;
		}
		return b;
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(remove_add_zero_unit(m[i])) b_ret = true;
	}
	return b_ret;
}

// Functions for handling log-based units
Unit *find_log_unit(const MathStructure &m, bool toplevel = true) {
	if(!toplevel && m.isUnit() && m.unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) m.unit())->hasNonlinearExpression() && (((AliasUnit*) m.unit())->expression().find("log") != string::npos || ((AliasUnit*) m.unit())->inverseExpression().find("log") != string::npos || ((AliasUnit*) m.unit())->expression().find("ln") != string::npos || ((AliasUnit*) m.unit())->inverseExpression().find("ln") != string::npos)) {
		return ((AliasUnit*) m.unit())->firstBaseUnit();
	}
	if(m.isMultiplication() && toplevel && m.last().isUnit()) {
		Unit *u = find_log_unit(m.last(), false);
		if(u) {
			for(size_t i = 0; i < m.size() - 1; i++) {
				if(m[i].containsType(STRUCT_UNIT, true)) return u;
			}
			return NULL;
		}
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return find_log_unit(((KnownVariable*) m.variable())->get(), toplevel);
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return NULL;
	for(size_t i = 0; i < m.size(); i++) {
		Unit *u = find_log_unit(m[i], false);
		if(u) return u;
	}
	return NULL;
}
bool separate_unit(MathStructure &m, Unit *u, const EvaluationOptions &eo) {
	if(m.isVariable() && m.variable()->isKnown()) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(mvar.contains(u, false, true, true)) {
			if(mvar.isMultiplication()) {
				bool b = false;
				for(size_t i = 0; i < mvar.size(); i++) {
					if(is_unit_multiexp(mvar[i])) {
						b = true;
					} else if(mvar[i].containsType(STRUCT_UNIT, false, true, true) != 0) {
						b = false;
						break;
					}
				}
				if(b) {
					m.transformById(FUNCTION_ID_STRIP_UNITS);
					for(size_t i = 0; i < mvar.size(); i++) {
						if(is_unit_multiexp(mvar[i])) {
							m.multiply(mvar[i], i);
						}
					}
					m.unformat(eo);
					separate_unit(m, u, eo);
					return true;
				}
			}
			if(eo.calculate_variables && ((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES) || (!m.variable()->isApproximate() && !mvar.containsInterval(true, false, false, 0, true)))) {
				m.set(mvar);
				m.unformat(eo);
				separate_unit(m, u, eo);
				return true;
			}
		}
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(separate_unit(m[i], u, eo)) {
			b = true;
		}
	}
	return b;
}
void separate_unit2(MathStructure &m, Unit *u, const EvaluationOptions &eo) {
	if(m.isMultiplication()) {
		size_t i_unit = m.size();
		for(size_t i = 0; i < m.size(); i++) {
			separate_unit2(m[i], u, eo);
			if(m[i].isUnit_exp()) {
				if(i_unit < 1 && ((m[i].isUnit() && m[i].unit() == u) || (m[i].isPower() && m[i][0].unit() == u))) {
					if(i_unit == i - 1) {
						m[i].multiply(m_one);
						m[i].swapChildren(1, 2);
					} else {
						m[i - 1].ref();
						m[i].multiply_nocopy(&m[i - 1]);
						m.delChild(i);
						i--;
					}
				}
				i_unit = i;
			} else if(i < m.size() && m[i].containsType(STRUCT_UNIT, false, true, true)) {
				MathStructure mtest(m[i]);
				CALCULATOR->beginTemporaryStopMessages();
				mtest.eval(eo);
				if(mtest.containsType(STRUCT_UNIT, false, true, true) > 0) {
					i_unit = i;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
		}
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			separate_unit2(m[i], u, eo);
		}
	}
}
void convert_log_units(MathStructure &m, const EvaluationOptions &eo) {
	while(true) {
		Unit *u = find_log_unit(m);
		if(!u) break;
		separate_unit(m, u, eo);
		separate_unit2(m, u, eo);
		if(!m.convert(u, true, NULL, false, eo)) break;
		CALCULATOR->error(false, "Log-based units were converted before calculation.", NULL);
	}
}

Unit *contains_temperature_unit(const MathStructure &m, bool only_cf = true, Unit *u_prev = NULL) {
	if(m.isUnit()) {
		if(!only_cf) {
			if(m.unit() != u_prev && (m.unit() == CALCULATOR->getUnitById(UNIT_ID_KELVIN) || m.unit()->containsRelativeTo(CALCULATOR->getUnitById(UNIT_ID_KELVIN)))) return m.unit();
		} else if(m.unit() == CALCULATOR->getUnitById(UNIT_ID_CELSIUS) || m.unit() == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT)) {
			return m.unit();
		}
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return contains_temperature_unit(((KnownVariable*) m.variable())->get(), only_cf, u_prev);
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return NULL;
	for(size_t i = 0; i < m.size(); i++) {
		Unit *u = contains_temperature_unit(m[i], only_cf, u_prev);
		if(u) return u;
	}
	return NULL;
}
bool separate_temperature_units(MathStructure &m, const EvaluationOptions &eo) {
	if(m.isVariable() && m.variable()->isKnown()) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(contains_temperature_unit(mvar, false)) {
			if(mvar.isMultiplication()) {
				bool b = false;
				for(size_t i = 0; i < mvar.size(); i++) {
					if(is_unit_multiexp(mvar[i])) {
						b = true;
					} else if(mvar[i].containsType(STRUCT_UNIT, false, true, true) != 0) {
						b = false;
						break;
					}
				}
				if(b) {
					m.transformById(FUNCTION_ID_STRIP_UNITS);
					for(size_t i = 0; i < mvar.size(); i++) {
						if(is_unit_multiexp(mvar[i])) {
							m.multiply(mvar[i], i);
						}
					}
					m.unformat(eo);
					separate_temperature_units(m, eo);
					return true;
				}
			}
			if(eo.calculate_variables && ((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES) || (!m.variable()->isApproximate() && !mvar.containsInterval(true, false, false, 0, true)))) {
				m.set(mvar);
				m.unformat(eo);
				separate_temperature_units(m, eo);
				return true;
			}
		}
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(separate_temperature_units(m[i], eo)) {
			b = true;
		}
	}
	return b;
}
void separate_temperature_units2(MathStructure &m, const EvaluationOptions &eo) {
	if(m.isMultiplication()) {
		size_t i_unit = m.size();
		for(size_t i = 0; i < m.size(); i++) {
			separate_temperature_units2(m[i], eo);
			if(m[i].isUnit_exp()) {
				if(i_unit < 1 && ((m[i].isUnit() && m[i].unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_KELVIN)) || (m[i].isPower() && m[i][0].unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_KELVIN)))) {
					if(i_unit == i - 1) {
						m[i].multiply(m_one);
						m[i].swapChildren(1, 2);
					} else {
						m[i - 1].ref();
						m[i].multiply_nocopy(&m[i - 1]);
						m.delChild(i);
						i--;
					}
				}
				i_unit = i;
			} else if(i < m.size() && m[i].containsType(STRUCT_UNIT, false, true, true)) {
				MathStructure mtest(m[i]);
				CALCULATOR->beginTemporaryStopMessages();
				mtest.eval(eo);
				if(mtest.containsType(STRUCT_UNIT, false, true, true) > 0) {
					i_unit = i;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
		}
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			separate_temperature_units2(m[i], eo);
		}
	}
}
void convert_temperature_units(MathStructure &m, const EvaluationOptions &eo) {
	if(CALCULATOR->getTemperatureCalculationMode() == TEMPERATURE_CALCULATION_RELATIVE || !CALCULATOR->getUnitById(UNIT_ID_KELVIN)) return;
	Unit *u = contains_temperature_unit(m, true);
	if(!u) return;
	if(!contains_temperature_unit(m, false, u)) {
		if(CALCULATOR->getTemperatureCalculationMode() == TEMPERATURE_CALCULATION_HYBRID) return;
		MathStructure *mp = &m;
		if(m.isMultiplication() && m.size() == 2 && m[0].isMinusOne()) mp = &m[1];
		if(mp->isUnit_exp()) return;
		if(mp->isMultiplication() && mp->size() > 0 && mp->last().isUnit_exp()) {
			bool b = false;
			for(size_t i = 0; i < mp->size() - 1; i++) {
				if(contains_temperature_unit((*mp)[i], true)) {b = true; break;}
			}
			if(!b) return;
		}
	}
	separate_temperature_units(m, eo);
	separate_temperature_units2(m, eo);
	m.convert(CALCULATOR->getUnitById(UNIT_ID_KELVIN), true, NULL, false, eo);
}

bool warn_ratio_units(MathStructure &m, bool top_level = true) {
	if(!top_level && m.isUnit() && ((m.unit()->subtype() == SUBTYPE_BASE_UNIT && m.unit()->referenceName() == "Np") || (m.unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) m.unit())->baseUnit()->referenceName() == "Np"))) {
		CALCULATOR->error(true, "Logarithmic ratio units is treated as other units and the result might not be as expected.", NULL);
		return true;
	}
	if(m.isMultiplication() && top_level && m.last().isUnit()) {
		if(m.size() < 2) return false;
		for(size_t i = 0; i < m.size() - 1; i++) {
			if(warn_ratio_units(m[i], false)) return true;
		}
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			if(warn_ratio_units(m[i], false)) return true;
		}
	}
	return false;
}

void clean_multiplications(MathStructure &mstruct) {
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication()) {
				size_t i2 = 0;
				for(; i2 < mstruct[i + i2].size(); i2++) {
					mstruct[i + i2][i2].ref();
					mstruct.insertChild_nocopy(&mstruct[i + i2][i2], i + i2 + 1);
				}
				mstruct.delChild(i + i2 + 1);
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		clean_multiplications(mstruct[i]);
	}
}

// Attempt to isolate x and test validity afterwards
bool try_isolate_x(MathStructure &mstruct, EvaluationOptions &eo3, const EvaluationOptions &eo) {
	if(mstruct.isProtected()) return false;
	if(mstruct.isComparison()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mtest(mstruct);
		eo3.test_comparisons = false;
		eo3.warn_about_denominators_assumed_nonzero = false;
		mtest[0].calculatesub(eo3, eo);
		mtest[1].calculatesub(eo3, eo);
		eo3.test_comparisons = eo.test_comparisons;
		const MathStructure *x_var2;
		if(eo.isolate_var) x_var2 = eo.isolate_var;
		else x_var2 = &mstruct.find_x_var();
		if(x_var2->isUndefined() || (mtest[0] == *x_var2 && !mtest[1].contains(*x_var2))) {
			CALCULATOR->endTemporaryStopMessages();
			 return false;
		}
		if(mtest.isolate_x(eo3, eo, *x_var2, false)) {
			if(test_comparisons(mstruct, mtest, *x_var2, eo3) >= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct = mtest;
				return true;
			}
		}
		CALCULATOR->endTemporaryStopMessages();
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(try_isolate_x(mstruct[i], eo3, eo)) b = true;
		}
		return b;
	}
	return false;
}

bool MathStructure::simplify(const EvaluationOptions &eo, bool unfactorize) {
	if(SIZE == 0) return false;
	if(unfactorize) {
		unformat();
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		eo2.combine_divisions = false;
		eo2.sync_units = false;
		calculatesub(eo2, eo2);
		bool b = do_simplification(*this, eo2, true, false, false);
		return combination_factorize(*this) || b;
	}
	return combination_factorize(*this);
}
bool MathStructure::expand(const EvaluationOptions &eo, bool unfactorize) {
	if(SIZE == 0) return false;
	EvaluationOptions eo2 = eo;
	eo2.sync_units = false;
	eo2.expand = true;
	if(unfactorize) calculatesub(eo2, eo2);
	do_simplification(*this, eo2, true, false, false);
	return false;
}
bool MathStructure::structure(StructuringMode structuring, const EvaluationOptions &eo, bool restore_first) {
	switch(structuring) {
		case STRUCTURING_NONE: {
			if(restore_first) {
				EvaluationOptions eo2 = eo;
				eo2.sync_units = false;
				calculatesub(eo2, eo2);
			}
			return false;
		}
		case STRUCTURING_FACTORIZE: {
			return factorize(eo, restore_first, 3, 0, true, 2, NULL, m_undefined, true, false, -1);
		}
		default: {
			return simplify(eo, restore_first);
		}
	}
}

bool merge_uncertainty(MathStructure &m, MathStructure &munc, const EvaluationOptions &eo3) {
	bool b_failed = true;
	if(munc.isVector() && m.isVector() && munc.size() == m.size()) {
		b_failed = false;
		for(size_t i = 0; i < m.size(); i++) {
			if(!merge_uncertainty(m[i], munc[i], eo3)) b_failed = true;
		}
		m.childrenUpdated();
		return !b_failed;
	}
	if(munc.isFunction() && munc.function()->id() == FUNCTION_ID_ABS && munc.size() == 1) {
		munc.setToChild(1);
	}
	bool one_prepended = false;
	test_munc:
	if(munc.isNumber()) {
		if(munc.isZero()) {
			return true;
		} else if(m.isNumber()) {
			m.number().setUncertainty(munc.number());
			m.numberUpdated();
			return true;
		} else if(m.isAddition()) {
			for(size_t i = 0; i < m.size(); i++) {
				if(m[i].isNumber()) {
					b_failed = false;
					m[i].number().setUncertainty(munc.number());
					m[i].numberUpdated();
					m.childUpdated(i + 1);
					break;
				}
			}
		} else if(m.isMultiplication() && m.size() == 2 && m[0].isNumber() && m.last().isUnit() && m.last().unit() == CALCULATOR->getRadUnit()) {
			m[0].number().setUncertainty(munc.number());
			m[0].numberUpdated();
			m.childUpdated(1);
			return true;
		}
	} else {
		if(munc.isMultiplication()) {
			if(!munc[0].isNumber()) {
				munc.insertChild(m_one, 1);
				one_prepended = true;
			}
		} else {
			munc.transform(STRUCT_MULTIPLICATION);
			munc.insertChild(m_one, 1);
			one_prepended = true;
		}
		if(munc.isMultiplication()) {
			if(munc.size() == 2) {
				if(m.isMultiplication() && m[0].isNumber() && (munc[1] == m[1] || (munc[1].isFunction() && munc[1].function()->id() == FUNCTION_ID_ABS && munc[1].size() == 1 && m[1] == munc[1][0]))) {
					m[0].number().setUncertainty(munc[0].number());
					m[0].numberUpdated();
					m.childUpdated(1);
					b_failed = false;
				} else if(m.equals(munc[1]) || (munc[1].isFunction() && munc[1].function()->id() == FUNCTION_ID_ABS && munc[1].size() == 1 && m.equals(munc[1][0]))) {
					m.transform(STRUCT_MULTIPLICATION);
					m.insertChild(m_one, 1);
					m[0].number().setUncertainty(munc[0].number());
					m[0].numberUpdated();
					m.childUpdated(1);
					b_failed = false;
				}
			} else if(m.isMultiplication()) {
				size_t i2 = 0;
				if(m[0].isNumber()) i2++;
				if(m.size() + 1 - i2 != munc.size() && m.last().isUnit() && m.last().unit() == CALCULATOR->getRadUnit()) {
					munc *= CALCULATOR->getRadUnit();
				}
				if(m.size() + 1 - i2 == munc.size()) {
					bool b = true;
					for(size_t i = 1; i < munc.size(); i++, i2++) {
						if(!munc[i].equals(m[i2]) && !(munc[i].isFunction() && munc[i].function()->id() == FUNCTION_ID_ABS && munc[i].size() == 1 && m[i2] == munc[i][0])) {
							b = false;
							break;
						}
					}
					if(b) {
						if(!m[0].isNumber()) {
							m.insertChild(m_one, 1);
						}
						m[0].number().setUncertainty(munc[0].number());
						m[0].numberUpdated();
						m.childUpdated(1);
						b_failed = false;
					}
				}
			}
			if(b_failed) {
				bool b = false;
				for(size_t i = 0; i < munc.size(); i++) {
					if(munc[i].isFunction() && munc[i].function()->id() == FUNCTION_ID_ABS && munc[i].size() == 1) {
						munc[i].setToChild(1);
						b = true;
					}
				}
				if(b) {
					munc.eval(eo3);
					goto test_munc;
				}
			}
		}
	}
	if(b_failed && one_prepended && munc.isMultiplication() && munc[0].isOne()) munc.delChild(1, true);
	return !b_failed;
}

bool separate_vector_vars(MathStructure &m, const EvaluationOptions &eo, vector<KnownVariable*> &vars, vector<MathStructure> &values) {
	if(m.isVariable() && m.variable()->isKnown() && (!m.variable()->isApproximate() || eo.approximation == APPROXIMATION_TRY_EXACT || eo.approximation == APPROXIMATION_APPROXIMATE)) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(mvar.isVector() && mvar.containsInterval(true, false, false, 1, true)) {
			bool b = false;
			for(size_t i = 0; i < vars.size(); i++) {
				if(vars[i] == m.variable()) {
					m = values[i];
					b = true;
					break;
				}
			}
			if(!b) {
				KnownVariable *mv = (KnownVariable*) m.variable();
				m.clearVector();
				for(size_t i = 0; i < mvar.size(); i++) {
					if(mvar[i].containsInterval(true, false, false, 1, true)) {
						KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mvar[i]) + ")", mvar[i]);
						m.addChild(v);
						v->ref();
						v->destroy();
					} else {
						m.addChild(mvar[i]);
					}
					separate_vector_vars(m[i], eo, vars, values);
				}
				vars.push_back(mv);
				values.push_back(m);
			}
			return true;
		}
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(separate_vector_vars(m[i], eo, vars, values)) {
			b_ret = true;
			m.childUpdated(i + 1);
		}
	}
	return b_ret;
}

#define FORMAT_COMPLEX_NUMBERS	if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo); \
				else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo); \
				else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);

void replace_aborted_variables(MathStructure &m) {
	if(m.isVariable() && m.variable()->isKnown() && !m.variable()->isRegistered() && m.variable()->referenceName().find(CALCULATOR->abortedMessage())) {
		m.set(((KnownVariable*) m.variable())->get());
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_aborted_variables(m[i]);
	}
}

MathStructure &MathStructure::eval(const EvaluationOptions &eo) {

	if(m_type == STRUCT_NUMBER) {FORMAT_COMPLEX_NUMBERS; return *this;}

	if(eo.structuring != STRUCTURING_NONE) warn_ratio_units(*this);

	unformat(eo);

	if(CALCULATOR->aborted() || m_type == STRUCT_UNDEFINED || m_type == STRUCT_ABORTED || m_type == STRUCT_DATETIME || m_type == STRUCT_UNIT || m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && !o_variable->isKnown())) return *this;

	if(eo.structuring != STRUCTURING_NONE && eo.sync_units) {
		convert_log_units(*this, eo);
		convert_temperature_units(*this, eo);
	}

	EvaluationOptions feo = eo;
	feo.structuring = STRUCTURING_NONE;
	feo.do_polynomial_division = false;
	feo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	EvaluationOptions eo2 = eo;
	eo2.structuring = STRUCTURING_NONE;
	eo2.expand = false;
	eo2.test_comparisons = false;
	eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	eo2.isolate_x = false;

	if(m_type == STRUCT_NUMBER) {FORMAT_COMPLEX_NUMBERS; return *this;}

	if(eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, 0);
		if(((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) && containsInterval(true, true, false, 1, true)) || (eo.sync_units && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.approximation != APPROXIMATION_EXACT && sync_approximate_units(*this, eo)) || (eo.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, true, true, false, 1, true))) {
			EvaluationOptions eo3 = eo2;
			eo3.split_squares = false;
			eo3.assume_denominators_nonzero = false;
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			vector<KnownVariable*> vars;
			vector<MathStructure> uncs;
			calculatesub(eo3, eo3);
			while(eo.sync_units && (separate_unit_vars(*this, feo, true) || sync_approximate_units(*this, feo, &vars, &uncs, false))) {
				calculatesub(eo3, eo3);
			}
			eo3.approximation = APPROXIMATION_APPROXIMATE;
			if(eo.sync_units) {
				sync_approximate_units(*this, feo, &vars, &uncs, true);
			}
			factorize_variables(*this, eo3);
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			eo3.expand = eo.expand;
			eo3.assume_denominators_nonzero = eo.assume_denominators_nonzero;
			solve_intervals(*this, eo3, feo);
		}
		if(eo.calculate_functions) calculate_differentiable_functions(*this, feo);
	} else if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA) {
		if((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) && containsInterval(true, true, false, 1, true)) {
			vector<KnownVariable*> vars;
			vector<MathStructure> uncs;
			separate_vector_vars(*this, feo, vars, uncs);
		}
		if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, -1);
		if(!isNumber() && (((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) && containsInterval(true, true, false, 1, true)) || containsInterval(true, false, false, 1, true) || (eo.sync_units && eo.approximation != APPROXIMATION_EXACT && sync_approximate_units(*this, eo)) || (eo.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, true, true, false, 1, true)))) {

			// calculate non-differentiable functions (not handled by variance formula) and functions without uncertainties
			if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, (eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) ? 2 : 1);
			if(CALCULATOR->aborted()) return *this;
			MathStructure munc, mbak(*this);
			if(eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) {
				// replace intervals with variables and calculate exact
				find_interval_replace_var_nr(*this);
				EvaluationOptions eo3 = eo2;
				eo3.split_squares = false;
				if(eo.expand && eo.expand >= -1) eo3.expand = -1;
				eo3.assume_denominators_nonzero = eo.assume_denominators_nonzero;
				eo3.approximation = APPROXIMATION_EXACT;
				vector<KnownVariable*> vars;
				vector<MathStructure> uncs;
				separate_vector_vars(*this, feo, vars, uncs);
				vars.clear();
				uncs.clear();
				calculatesub(eo3, eo3);
				// remove units from variables with uncertainties
				while(eo.sync_units && (separate_unit_vars(*this, feo, true) || sync_approximate_units(*this, feo, &vars, &uncs, false))) {
					calculatesub(eo3, eo3);
				}
			}
			bool b_failed = false;
			if(containsType(STRUCT_COMPARISON)) {

				// Handle comparisons by exact calculation followed by calculation of each side of comparisons separately

				EvaluationOptions eo3 = eo;
				eo3.approximation = APPROXIMATION_EXACT;
				eo3.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				eo3.structuring = STRUCTURING_NONE;
				eo3.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;

				// replace uncertain values with variables
				vector<Variable*> vars;
				while(true) {
					Variable *v = NULL;
					Variable *uv = find_interval_replace_var_comp(*this, eo3, &v);
					if(!uv) break;
					if(v) replace(v, uv);
					vars.push_back(uv);
				}

				eval(eo3);

				// restore uncertain values
				for(size_t i = 0; i < vars.size(); i++) {
					if(vars[i]->isKnown()) replace(vars[i], ((KnownVariable*) vars[i])->get());
					else replace(vars[i], ((UnknownVariable*) vars[i])->interval());
					vars[i]->destroy();
				}

				if(CALCULATOR->aborted()) {replace_aborted_variables(*this); return *this;}

				// calculate each side of comparisons separately
				if(eval_comparison_sides(*this, feo)) {
					if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_roots(*this, eo);}
					structure(eo.structuring, eo2, false);
					if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_roots(*this, eo);}
					clean_multiplications(*this);
				} else if(CALCULATOR->aborted()) {
					replace_aborted_variables(*this);
				} else {
					CALCULATOR->error(false, _("Calculation of uncertainty propagation partially failed (using interval arithmetic instead when necessary)."), NULL);
					EvaluationOptions eo4 = eo;
					eo4.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
					eval(eo4);
				}
				return *this;

			} else {

				CALCULATOR->beginTemporaryStopMessages();

				MathStructure mtest(*this);

				if(!representsScalar()) b_failed = true;

				// calculate uncertainty
				if(m_type == STRUCT_VECTOR) {
					munc.clearVector();
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isVector()) {
							munc.addChild(m_zero);
							if(CHILD(i).size() > 0) munc[i].clearVector();
							for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
								munc[i].addChild(calculate_uncertainty(CHILD(i)[i2], eo, b_failed));
								if(b_failed) break;
							}
						} else {
							munc.addChild(calculate_uncertainty(CHILD(i), eo, b_failed));
							if(b_failed) break;
						}
					}
				} else {
					munc = calculate_uncertainty(*this, eo, b_failed);
				}

				if(b_failed) {
					CALCULATOR->endTemporaryStopMessages();
					b_failed = false;
					EvaluationOptions eo3 = eo2;
					eo3.split_squares = false;
					if(eo.expand && eo.expand >= -1) eo3.expand = -1;
					eo3.assume_denominators_nonzero = eo.assume_denominators_nonzero;
					eo3.approximation = APPROXIMATION_EXACT;
					CALCULATOR->beginTemporaryStopMessages();
					mtest.calculatesub(eo3, eo3);
					if(mtest.isVector()) {
						munc.clearVector();
						for(size_t i = 0; i < mtest.size(); i++) {
							if(mtest[i].isVector()) {
								munc.addChild(m_zero);
								if(mtest[i].size() > 0) munc[i].clearVector();
								for(size_t i2 = 0; i2 < mtest[i].size(); i2++) {
									munc[i].addChild(calculate_uncertainty(mtest[i][i2], eo, b_failed));
									if(b_failed) break;
								}
							} else {
								munc.addChild(calculate_uncertainty(mtest[i], eo, b_failed));
								if(b_failed) break;
							}
						}
					} else {
						munc = calculate_uncertainty(mtest, eo, b_failed);
					}
					if(!b_failed) {
						set_nocopy(mtest);
					}
				}

				if(!b_failed && !munc.isZero()) {

					// evaluate uncertainty and expression without uncertainty
					EvaluationOptions eo3 = eo;
					eo3.keep_zero_units = false;
					eo3.structuring = STRUCTURING_NONE;
					eo3.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					eo3.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
					if(eo3.approximation == APPROXIMATION_TRY_EXACT) eo3.approximation = APPROXIMATION_APPROXIMATE;
					munc.eval(eo3);
					eo3.keep_zero_units = eo.keep_zero_units;
					eval(eo3);

					// Add uncertainty to calculated value
					if(eo.keep_zero_units) remove_add_zero_unit(*this);
					b_failed = !merge_uncertainty(*this, munc, eo3);

					if(b_failed && munc.countTotalChildren(false) < 60) {
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_ln(munc); simplify_roots(*this, eo); simplify_roots(munc, eo);}
						structure(eo.structuring, eo2, false);
						munc.structure(eo.structuring, eo2, false);
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_ln(munc); simplify_roots(*this, eo); simplify_roots(munc, eo);}
						clean_multiplications(*this);
						clean_multiplications(munc);
						transformById(FUNCTION_ID_UNCERTAINTY);
						addChild(munc);
						addChild(m_zero);
						CALCULATOR->endTemporaryStopMessages(true);
						return *this;
					}
					if(!b_failed) {
						CALCULATOR->endTemporaryStopMessages(true);
						if(m_type == STRUCT_NUMBER) return *this;
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_roots(*this, eo);}
						structure(eo.structuring, eo2, false);
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_roots(*this, eo);}
						clean_multiplications(*this);
						return *this;
					}
				}
				CALCULATOR->endTemporaryStopMessages(!b_failed);
				if(b_failed) {
					set(mbak);
					if(CALCULATOR->aborted()) {replace_aborted_variables(*this); return *this;}
					CALCULATOR->error(false, _("Calculation of uncertainty propagation failed (using interval arithmetic instead)."), NULL);
					EvaluationOptions eo3 = eo;
					eo3.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
					eval(eo3);
					return *this;
				}
			}
		}
		if(eo.calculate_functions) calculate_differentiable_functions(*this, feo);
	} else if(eo.calculate_functions) {
		calculateFunctions(feo);
	}

	if(m_type == STRUCT_NUMBER) {FORMAT_COMPLEX_NUMBERS; return *this;}

	if(eo2.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC || eo2.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;

	// do exact calculations first and handle variables with intervals
	if(eo2.approximation == APPROXIMATION_TRY_EXACT || (eo2.approximation == APPROXIMATION_APPROXIMATE && (containsUnknowns() || containsInterval(false, true, false, 0)))) {
		EvaluationOptions eo3 = eo2;
		if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
		else eo3.approximation = APPROXIMATION_EXACT;
		eo3.split_squares = false;
		eo3.assume_denominators_nonzero = false;
		calculatesub(eo3, feo);
		if(m_type == STRUCT_NUMBER) {FORMAT_COMPLEX_NUMBERS; return *this;}
		if(eo.interval_calculation < INTERVAL_CALCULATION_INTERVAL_ARITHMETIC && eo.expand && eo.expand >= -1 && !containsType(STRUCT_COMPARISON, true, true, true)) {
			unformat(eo);
			eo3.expand = -1;
			calculatesub(eo3, feo);
		}
		eo3.approximation = APPROXIMATION_APPROXIMATE;
		if(eo2.interval_calculation >= INTERVAL_CALCULATION_INTERVAL_ARITHMETIC && containsInterval(false, true, false, -1)) factorize_variables(*this, eo3);
		if(containsType(STRUCT_COMPARISON) && containsInterval(false, true, false, false)) {
			eo3.approximation = APPROXIMATION_EXACT;
			fix_eqs(*this, eo3);
		}
		eo2.approximation = APPROXIMATION_APPROXIMATE;
	} else if(eo2.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, false, true, false, -1)) {
		EvaluationOptions eo3 = eo2;
		eo3.split_squares = false;
		eo3.assume_denominators_nonzero = false;
		calculatesub(eo3, feo);
		eo3.approximation = APPROXIMATION_APPROXIMATE;
		if(eo2.interval_calculation >= INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) factorize_variables(*this, eo3, true);
	}
	if(eo2.approximation == APPROXIMATION_EXACT) replace_function_vars(*this);

	calculatesub(eo2, feo);

	if(m_type == STRUCT_NUMBER) {FORMAT_COMPLEX_NUMBERS; return *this;}
	if(m_type == STRUCT_UNDEFINED || m_type == STRUCT_ABORTED || m_type == STRUCT_DATETIME || m_type == STRUCT_UNIT || m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && !o_variable->isKnown())) return *this;
	if(CALCULATOR->aborted()) return *this;

	eo2.sync_units = false;
	eo2.isolate_x = eo.isolate_x;

	// Try isolate x without expanding expression
	if(eo2.isolate_x) {
		eo2.assume_denominators_nonzero = false;
		if(isolate_x(eo2, feo)) {
			if(CALCULATOR->aborted()) return *this;
			if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
			calculatesub(eo2, feo);
		} else {
			if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
		}
		if(CALCULATOR->aborted()) return *this;
	}

	// Try isolate x after expanding expression and perform simplification using gcd and polynomial division
	if(eo.expand != 0 || (eo.test_comparisons && containsType(STRUCT_COMPARISON))) {
		eo2.test_comparisons = eo.test_comparisons;
		eo2.expand = eo.expand;
		if(eo2.expand && (!eo.test_comparisons || !containsType(STRUCT_COMPARISON))) eo2.expand = -2;
		bool b = eo2.test_comparisons;
		if(!b && isAddition()) {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).containsType(STRUCT_ADDITION, false) == 1) {
					b = true;
					break;
				}
			}
		} else if(!b) {
			b = containsType(STRUCT_ADDITION, false) == 1;
		}
		if(b) {
			calculatesub(eo2, feo);
			if(CALCULATOR->aborted()) return *this;
			if(eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
			if(CALCULATOR->aborted()) return *this;
			if(eo2.isolate_x) {
				eo2.assume_denominators_nonzero = false;
				if(isolate_x(eo2, feo)) {
					if(CALCULATOR->aborted()) return *this;
					if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
					calculatesub(eo2, feo);
					if(containsType(STRUCT_ADDITION, false) == 1 && eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
				} else {
					if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
				}
				if(CALCULATOR->aborted()) return *this;
			}
		}
	}

	// Final attempt to isolate x, by assuming denominators is non zero and testing validity afterwards
	if(eo2.isolate_x && containsType(STRUCT_COMPARISON) && eo2.assume_denominators_nonzero) {
		eo2.assume_denominators_nonzero = 2;
		if(try_isolate_x(*this, eo2, feo)) {
			if(CALCULATOR->aborted()) return *this;
			calculatesub(eo2, feo);
			if(containsType(STRUCT_ADDITION, false) == 1 && eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
		}
	}

	simplify_functions(*this, eo2, feo);

	if(CALCULATOR->aborted()) return *this;

	if(eo.structuring != STRUCTURING_NONE) {

		if(!DEFAULT_RADIANS(eo.parse_options.angle_unit)) convert_to_default_angle_unit(*this, eo);
		simplify_ln(*this);
		simplify_roots(*this, eo);

		if(eo.keep_zero_units) remove_add_zero_unit(*this);

		structure(eo.structuring, eo2, false);

		simplify_ln(*this);
		simplify_roots(*this, eo);

	}

	clean_multiplications(*this);

	FORMAT_COMPLEX_NUMBERS

	return *this;
}

