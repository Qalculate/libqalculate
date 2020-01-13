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

bool MathStructure::decomposeFractions(const MathStructure &x_var, const EvaluationOptions &eo) {
	MathStructure mtest2;
	bool b = false;
	int mmul_i = 0;
	if(isPower()) {
		if(CHILD(1).isMinusOne() && CHILD(0).isMultiplication() && CHILD(0).size() >= 2) {
			mtest2 = CHILD(0);
			b = true;
		}
	} else if(isMultiplication() && SIZE == 2) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isPower() && CHILD(i)[1].isMinusOne() && (CHILD(i)[0].isPower() || CHILD(i)[0].isMultiplication())) {
				mtest2 = CHILD(i)[0];
				b = true;
			} else if(CHILD(i) == x_var) {
				mmul_i = 1;
			} else if(CHILD(i).isPower() && CHILD(i)[0] == x_var && CHILD(i)[1].isInteger() && CHILD(i)[1].number().isPositive() && CHILD(i)[1].number().isLessThan(100)) {
				mmul_i = CHILD(i)[1].number().intValue();
			}
		}
		if(mmul_i == 0) b = false;
	}
	if(b) {
		if(!mtest2.isMultiplication()) mtest2.transform(STRUCT_MULTIPLICATION);
		MathStructure mfacs, mnew;
		mnew.setType(STRUCT_ADDITION);
		mfacs.setType(STRUCT_ADDITION);
		vector<int> i_degrees;
		i_degrees.resize(mtest2.size(), 1);
		for(size_t i = 0; i < mtest2.size() && b; i++) {
			if(CALCULATOR->aborted()) return false;
			MathStructure mfactor = mtest2[i];
			if(mtest2[i].isPower()) {
				if(!mtest2[i][1].isInteger() || !mtest2[i][1].number().isPositive() || mtest2[i][1].isOne() || mtest2[i][1].number().isGreaterThan(100)) {
					b = false;
					break;
				}
				mfactor = mtest2[i][0];
			}
			if(mfactor.isAddition()) {
				bool b2 = false;
				for(size_t i2 = 0; i2 < mfactor.size() && b; i2++) {
					if(mfactor[i2].isMultiplication()) {
						bool b_x = false;
						for(size_t i3 = 0; i3 < mfactor[i2].size() && b; i3++) {
							if(!b_x && mfactor[i2][i3].isPower() && mfactor[i2][i3][0] == x_var) {
								if(!mfactor[i2][i3][1].isInteger() || !mfactor[i2][i3][1].number().isPositive() || mfactor[i2][i3][1].isOne() || mfactor[i2][i3][1].number().isGreaterThan(100)) {
									b = false;
								} else if(mfactor[i2][i3][1].number().intValue() > i_degrees[i]) {
									i_degrees[i] = mfactor[i2][i3][1].number().intValue();
								}
								b_x = true;
							} else if(!b_x && mfactor[i2][i3] == x_var) {
								b_x = true;
							} else if(mfactor[i2][i3].containsRepresentativeOf(x_var, true, true) != 0) {
								b = false;
							}
						}
						if(!b_x) b2 = true;
					} else if(mfactor[i2].isPower() && mfactor[i2][0] == x_var) {
						if(!mfactor[i2][1].isInteger() || !mfactor[i2][1].number().isPositive() || mfactor[i2][1].isOne() || mfactor[i2][1].number().isGreaterThan(100)) {
							b = false;
						} else if(mfactor[i2][1].number().intValue() > i_degrees[i]) {
							i_degrees[i] = mfactor[i2][1].number().intValue();
						}
					} else if(mfactor[i2] == x_var) {
					} else if(mfactor[i2].containsRepresentativeOf(x_var, true, true) != 0) {
						b = false;
					} else {
						b2 = true;
					}
				}
				if(!b2) b = false;
			} else if(mfactor != x_var) {
				b = false;
			}
		}
		MathStructure mtest3, mnums3;
		mnums3.clearVector();
		mtest3.clearVector();
		if(b) {
			UnknownVariable *var = new UnknownVariable("", string("a"));
			var->setAssumptions(new Assumptions());
			MathStructure mvar(var);
			for(size_t i = 0; i < mtest2.size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(i_degrees[i] == 1) {
					MathStructure mnum(1, 1, 0);
					if(mtest2.size() != 1) {
						mnum = mtest2;
						mnum.delChild(i + 1, true);
					}
					MathStructure mx(mtest2[i]);
					mx.transform(COMPARISON_EQUALS, m_zero);
					mx.replace(x_var, mvar);
					mx.isolate_x(eo, mvar);
					mx.calculatesub(eo, eo, true);
					if(mx.isLogicalOr()) mx.setToChild(1);
					if(!mx.isComparison() || mx.comparisonType() != COMPARISON_EQUALS || mx[0] != var) {b = false; break;}
					mx.setToChild(2);
					if(mtest2.size() != 1) {
						mfacs.addChild(mnum);
						mnum.replace(x_var, mx);
						mnum.inverse();
					}
					if(mmul_i > 0) {
						mx ^= Number(mmul_i, 1);
						if(mtest2.size() == 1) mnum = mx;
						else mnum *= mx;
					}
					mnum.calculatesub(eo, eo, true);
					if(mtest2.size() == 1) mfacs.addChild(mnum);
					else mfacs.last() *= mnum;
					mnums3.addChild(mnum);
					mtest3.addChild(mtest2[i]);
				}
			}
			var->destroy();
		}
		if(b) {
			vector<UnknownVariable*> vars;
			bool b_solve = false;
			for(size_t i = 0; i < mtest2.size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(mtest2[i].isPower()) {
					int i_exp = mtest2[i][1].number().intValue();
					for(int i_exp_n = mtest2[i][1].number().intValue() - (i_degrees[i] == 1 ? 1 : 0); i_exp_n > 0; i_exp_n--) {
						if(i_exp_n == 1) {
							mtest3.addChild(mtest2[i][0]);
						} else {
							mtest3.addChild(mtest2[i]);
							if(i_exp_n != i_exp) mtest3.last()[1].number() = i_exp_n;
						}
						if(i_exp ==  i_exp_n) {
							if(mtest2.size() > 1) {
								mfacs.addChild(mtest2);
								mfacs.last().delChild(i + 1, true);
							}
						} else {
							mfacs.addChild(mtest2);
							if(i_exp - i_exp_n == 1) mfacs.last()[i].setToChild(1);
							else mfacs.last()[i][1].number() = i_exp - i_exp_n;
						}
						if(i_degrees[i] == 1) {
							UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()));
							var->setAssumptions(new Assumptions());
							mnums3.addChild_nocopy(new MathStructure(var));
							vars.push_back(var);
						} else {
							mnums3.addChild_nocopy(new MathStructure());
							mnums3.last().setType(STRUCT_ADDITION);
							for(int i2 = 1; i2 <= i_degrees[i]; i2++) {
								UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()) + i2s(i2));
								var->setAssumptions(new Assumptions());
								if(i2 == 1) {
									mnums3.last().addChild_nocopy(new MathStructure(var));
								} else {
									mnums3.last().addChild_nocopy(new MathStructure(var));
									mnums3.last().last() *= x_var;
									if(i2 > 2) mnums3.last().last().last() ^= Number(i2 - 1, 1);
								}
								vars.push_back(var);
							}
						}
						if(i_exp != i_exp_n || mtest2.size() > 1) mfacs.last() *= mnums3.last();
						else mfacs.addChild(mnums3.last());
						b_solve = true;
					}
				} else if(i_degrees[i] > 1) {
					mtest3.addChild(mtest2[i]);
					if(mtest2.size() > 1) {
						mfacs.addChild(mtest2);
						mfacs.last().delChild(i + 1, true);
					}
					mnums3.addChild_nocopy(new MathStructure());
					mnums3.last().setType(STRUCT_ADDITION);
					for(int i2 = 1; i2 <= i_degrees[i]; i2++) {
						UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()) + i2s(i2));
						var->setAssumptions(new Assumptions());
						if(i2 == 1) {
							mnums3.last().addChild_nocopy(new MathStructure(var));
						} else {
							mnums3.last().addChild_nocopy(new MathStructure(var));
							mnums3.last().last() *= x_var;
							if(i2 > 2) mnums3.last().last().last() ^= Number(i2 - 1, 1);
						}
						vars.push_back(var);
					}
					if(mtest2.size() > 1) mfacs.last() *= mnums3.last();
					else mfacs.addChild(mnums3.last());
					b_solve = true;
				}
			}
			if(b_solve) {
				mfacs.childrenUpdated(true);
				MathStructure mfac_expand(mfacs);
				mfac_expand.calculatesub(eo, eo, true);
				size_t i_degree = 0;
				if(mfac_expand.isAddition()) {
					i_degree = mfac_expand.degree(x_var).uintValue();
					if(i_degree >= 100 || i_degree <= 0) b = false;
				}
				if(i_degree == 0) b = false;
				if(b) {
					MathStructure m_eqs;
					m_eqs.resizeVector(i_degree + 1, m_zero);
					for(size_t i = 0; i < m_eqs.size(); i++) {
						for(size_t i2 = 0; i2 < mfac_expand.size();) {
							if(CALCULATOR->aborted()) return false;
							bool b_add = false;
							if(i == 0) {
								if(!mfac_expand[i2].contains(x_var)) b_add = true;
							} else {
								if(mfac_expand[i2].isMultiplication() && mfac_expand[i2].size() >= 2) {
									for(size_t i3 = 0; i3 < mfac_expand[i2].size(); i3++) {
										if(i == 1 && mfac_expand[i2][i3] == x_var) b_add = true;
										else if(i > 1 && mfac_expand[i2][i3].isPower() && mfac_expand[i2][i3][0] == x_var && mfac_expand[i2][i3][1] == i) b_add = true;
										if(b_add) {
											mfac_expand[i2].delChild(i3 + 1, true);
											break;
										}
									}

								} else {
									if(i == 1 && mfac_expand[i2] == x_var) b_add = true;
									else if(i > 1 && mfac_expand[i2].isPower() && mfac_expand[i2][0] == x_var && mfac_expand[i2][1] == i) b_add = true;
									if(b_add) mfac_expand[i2].set(1, 1, 0);
								}
							}
							if(b_add) {
								if(m_eqs[i].isZero()) m_eqs[i] = mfac_expand[i2];
								else m_eqs[i].add(mfac_expand[i2], true);
								mfac_expand.delChild(i2 + 1);
							} else {
								i2++;
							}
						}
					}
					if(mfac_expand.size() == 0 && m_eqs.size() >= vars.size()) {
						for(size_t i = 0; i < m_eqs.size(); i++) {
							if(!m_eqs[i].isZero()) {
								m_eqs[i].transform(COMPARISON_EQUALS, i == (size_t) mmul_i ? m_one : m_zero);
							}
						}
						for(size_t i = 0; i < m_eqs.size();) {
							if(m_eqs[i].isZero()) {
								m_eqs.delChild(i + 1);
								if(i == (size_t) mmul_i) b = false;
							} else {
								i++;
							}
						}
						if(b) {
							MathStructure vars_m;
							vars_m.clearVector();
							for(size_t i = 0; i < vars.size(); i++) {
								vars_m.addChild_nocopy(new MathStructure(vars[i]));
							}
							for(size_t i = 0; i < m_eqs.size() && b; i++) {
								for(size_t i2 = 0; i2 < vars_m.size(); i2++) {
									if(m_eqs[i].isComparison() && m_eqs[i][0].contains(vars_m[i2], true)) {
										if(CALCULATOR->aborted() || m_eqs[i].countTotalChildren() > 1000) return false;
										m_eqs[i].isolate_x(eo, vars_m[i2]);
										if(CALCULATOR->aborted() || m_eqs[i].countTotalChildren() > 10000) return false;
										m_eqs[i].calculatesub(eo, eo, true);
										if(m_eqs[i].isLogicalOr()) m_eqs[i].setToChild(1);
										if(m_eqs[i].isComparison() && m_eqs[i].comparisonType() == COMPARISON_EQUALS && m_eqs[i][0] == vars_m[i2]) {
											for(size_t i3 = 0; i3 < m_eqs.size(); i3++) {
												if(i3 != i) {
													if(CALCULATOR->aborted()) return false;
													m_eqs[i3][0].calculateReplace(vars_m[i2], m_eqs[i][1], eo);
													if(CALCULATOR->aborted()) return false;
													m_eqs[i3][1].calculateReplace(vars_m[i2], m_eqs[i][1], eo);
												}
											}
											vars_m.delChild(i2 + 1);
										} else {
											b = false;
										}
										break;
									}
								}
							}
							for(size_t i = 0; i < m_eqs.size();) {
								if(CALCULATOR->aborted()) return false;
								m_eqs[i].calculatesub(eo, eo);
								if(m_eqs[i].isZero()) {b = false; break;}
								if(m_eqs[i].isOne()) m_eqs.delChild(i + 1);
								else i++;
							}
							if(b && vars_m.size() == 0) {
								for(size_t i = 0; i < vars.size(); i++) {
									for(size_t i2 = 0; i2 < m_eqs.size(); i2++) {
										if(m_eqs[i2][0] == vars[i]) {
											mnums3.replace(vars[i], m_eqs[i2][1]);
											break;
										}
									}
								}
							} else {
								b = false;
							}
						}
					} else {
						b = false;
					}
				}
			}
			for(size_t i = 0; i < vars.size(); i++) {
				vars[i]->destroy();
			}
			if(b) {
				for(size_t i = 0; i < mnums3.size(); i++) {
					mnums3.calculatesub(eo, eo, true);
					if(!mnums3[i].isZero()) {
						if(mnums3[i].isOne()) {
							mnew.addChild(mtest3[i]);
							mnew.last().inverse();
						} else {
							mnew.addChild(mnums3[i]);
							mnew.last() /= mtest3[i];
						}
					}
				}
			}
			if(b) {
				if(mnew.size() == 0) mnew.clear();
				else if(mnew.size() == 1) mnew.setToChild(1);
				mnew.childrenUpdated();
				if(equals(mnew, true)) return false;
				set(mnew, true);
				return true;
			}
		}
	}
	return false;
}

bool expand_partial_fractions(MathStructure &m, const EvaluationOptions &eo, bool do_simplify = true) {
	MathStructure mtest(m);
	if(do_simplify) {
		mtest.simplify(eo);
		mtest.calculatesub(eo, eo, true);
	}
	if(mtest.isPower() && mtest[1].representsNegative()) {
		MathStructure x_var = mtest[0].find_x_var();
		if(!x_var.isUndefined() && mtest[0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
			if(mtest.decomposeFractions(x_var, eo) && mtest.isAddition()) {
				m = mtest;
				return true;
			}
		}
	} else if(mtest.isMultiplication()) {
		for(size_t i = 0; i < mtest.size(); i++) {
			if(mtest[i].isPower() && mtest[i][1].isMinusOne() && mtest[i][0].isAddition()) {
				MathStructure x_var = mtest[i][0].find_x_var();
				if(!x_var.isUndefined()) {
					MathStructure mfac(mtest);
					mfac.delChild(i + 1, true);
					bool b_poly = true;
					if(mfac.contains(x_var, true)) {
						MathStructure mquo, mrem;
						b_poly = polynomial_long_division(mfac, mtest[i][0], x_var, mquo, mrem, eo, true);
						if(b_poly && !mquo.isZero()) {
							m = mquo;
							if(!mrem.isZero()) {
								m += mrem;
								m.last() *= mtest[i];
								m.childrenUpdated();
							}
							expand_partial_fractions(m, eo, false);
							return true;
						}
					}
					if(b_poly && mtest[i][0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
						MathStructure mmul(1, 1, 0);
						while(mtest[i][0].isMultiplication() && mtest[i][0].size() >= 2 && !mtest[i][0][0].containsRepresentativeOf(x_var, true)) {
							if(mmul.isOne()) {
								mmul = mtest[i][0][0];
								mmul.calculateInverse(eo);
							} else {
								mmul *= mtest[i][0][0];
								mmul.last().calculateInverse(eo);
								mmul.calculateMultiplyLast(eo);
							}
							mtest[i][0].delChild(1, true);
						}
						for(size_t i2 = 0; i2 < mtest.size();) {
							if(i2 != i && !mtest[i2].containsRepresentativeOf(x_var, true)) {
								if(mmul.isOne()) {
									mmul = mtest[i2];
								} else {
									mmul.calculateMultiply(mtest[i2], eo);
								}
								if(mtest.size() == 2) {
									mtest.setToChild(i + 1, true);
									break;
								} else {
									mtest.delChild(i2 + 1);
									if(i2 < i) i--;
								}
							} else {
								i2++;
							}
						}
						if(mtest.decomposeFractions(x_var, eo) && mtest.isAddition()) {
							m = mtest;
							if(!mmul.isOne()) {
								for(size_t i2 = 0; i2 < m.size(); i2++) {
									m[i2].calculateMultiply(mmul, eo);
								}
							}
							return true;
						}
					}
				}
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mtest.size(); i++) {
			if(expand_partial_fractions(mtest[i], eo, false)) {
				b = true;
			}
		}
		if(b) {
			m = mtest;
			m.calculatesub(eo, eo, false);
			return true;
		}
	}
	return false;
}
bool MathStructure::expandPartialFractions(const EvaluationOptions &eo) {
	return expand_partial_fractions(*this, eo, true);
}

