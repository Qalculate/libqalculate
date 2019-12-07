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
#include "MathStructure-support.h"
#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool warn_test_interval(MathStructure &mnonzero, const EvaluationOptions &eo2) {
	if(mnonzero.isComparison() && mnonzero[0].isVariable() && !mnonzero[0].variable()->isKnown() && !((UnknownVariable*) mnonzero[0].variable())->interval().isUndefined()) {
		/*if(((UnknownVariable*) mnonzero[0].variable())->interval().isNumber() && mnonzero[1].isNumber()) {
			MathStructure mbak(mnonzero);
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			if(!mnonzero.isNumber()) mnonzero.clear();
			return true;
		} else*/ if(((UnknownVariable*) mnonzero[0].variable())->interval().containsInterval(true)) {
			MathStructure mbak(mnonzero);
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			if(mnonzero.isComparison()) mnonzero = mbak;
			else return true;
		} else {
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			return true;
		}
	} else if(mnonzero.isLogicalAnd() || mnonzero.isLogicalOr()) {
		bool b_ret = false;
		for(size_t i = 0; i < mnonzero.size(); i++) {
			if(warn_test_interval(mnonzero[i], eo2)) b_ret = true;
		}
		if(b_ret) {
			mnonzero.calculatesub(eo2, eo2, false);
			return true;
		}
	}
	return false;
}
bool warn_about_assumed_not_value(const MathStructure &mstruct, const MathStructure &mvalue, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(mvalue, OPERATION_NOT_EQUALS);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mvalue.isZero() && mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("Required assumption: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero(const MathStructure &mstruct, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_or_positive(const MathStructure &mstruct, const MathStructure &mstruct2, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_llgg(const MathStructure &mstruct, const MathStructure &mstruct2, const MathStructure &mstruct3, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	MathStructure *mpos2 = new MathStructure(mstruct3);
	mpos2->add(m_zero, OPERATION_EQUALS_GREATER);
	mpos->add_nocopy(mpos2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	MathStructure *mneg = new MathStructure(mstruct2);
	mneg->add(m_zero, OPERATION_LESS);
	MathStructure *mneg2 = new MathStructure(mstruct3);
	mneg2->add(m_zero, OPERATION_LESS);
	mneg->add_nocopy(mneg2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mneg, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}

int MathStructure::merge_addition(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.add(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(isZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(mstruct.isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(mstruct.representsReal(false)) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(representsReal(false)) {
			if(mparent) {
				mparent->swapChildren(index_this + 1, index_mstruct + 1);
			} else {
				clear(true);
				o_number = mstruct.number();
				MERGE_APPROX_AND_PREC(mstruct)
			}
			return 3;
		}
	}
	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {return 0;}
				case STRUCT_VECTOR: {
					if(SIZE == mstruct.size()) {
						for(size_t i = 0; i < SIZE; i++) {
							CHILD(i).calculateAdd(mstruct[i], eo, this, i);
						}
						MERGE_APPROX_AND_PREC(mstruct)
						CHILDREN_UPDATED
						return 1;
					}
				}
				default: {
					if(mstruct.representsScalar()) {
						for(size_t i = 0; i < SIZE; i++) {
							CHILD(i).calculateAdd(mstruct, eo, this, i);
						}
						CHILDREN_UPDATED
						return 1;
					}
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_ADDITION: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							INSERT_REF(&mstruct[i], i)
							calculateAddIndex(i, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateAddLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
				default: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(reversed) {
						PREPEND_REF(&mstruct);
						calculateAddIndex(0, eo, true, mparent, index_this);
					} else {
						APPEND_REF(&mstruct);
						calculateAddLast(eo, true, mparent, index_this);
					}
					return 1;
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_ADDITION: {
					return 0;
				}
				case STRUCT_MULTIPLICATION: {

					size_t i1 = 0, i2 = 0;
					bool b = true;
					if(CHILD(0).isNumber()) i1 = 1;
					if(mstruct[0].isNumber()) i2 = 1;
					if(SIZE - i1 == mstruct.size() - i2) {
						for(size_t i = i1; i < SIZE; i++) {
							if(CHILD(i) != mstruct[i + i2 - i1]) {
								b = false;
								break;
							}
						}
						if(b) {
							if(i1 == 0) {
								PREPEND(m_one);
							}
							if(i2 == 0) {
								CHILD(0).number()++;
							} else {
								CHILD(0).number() += mstruct[0].number();
							}
							MERGE_APPROX_AND_PREC(mstruct)
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
							return 1;
						}
					}

					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(eo.transform_trigonometric_functions && CHILD(i2).isPower() && CHILD(i2)[0].isFunction() && (CHILD(i2)[0].function()->id() == FUNCTION_ID_COS || CHILD(i2)[0].function()->id() == FUNCTION_ID_SIN) && CHILD(i2)[0].size() == 1 && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isTwo() && mstruct.size() > 0) {
							if(!FUNCTION_PROTECTED(eo, CHILD(i2)[0].function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN)) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(mstruct[i].isPower() && mstruct[i][0].isFunction() && mstruct[i][0].function()->id() == (CHILD(i2)[0].function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN) && mstruct[i][0].size() == 1 && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && CHILD(i2)[0][0] == mstruct[i][0][0]) {
										MathStructure madd1(*this);
										MathStructure madd2(mstruct);
										MathStructure madd;
										madd1.delChild(i2 + 1, true);
										madd2.delChild(i + 1, true);
										if(CHILD(i2)[0].function()->id() == FUNCTION_ID_SIN) {madd = madd2; madd2.calculateNegate(eo);}
										else {madd = madd1; madd1.calculateNegate(eo);}
										if(madd1.calculateAdd(madd2, eo)) {
											SET_CHILD_MAP(i2);
											CHILD(0).setFunctionId(FUNCTION_ID_SIN);
											calculateMultiply(madd1, eo);
											EvaluationOptions eo2 = eo;
											eo2.transform_trigonometric_functions = false;
											calculateAdd(madd, eo2);
											return 1;
										}
									}
								}
							}
							if(eo.protected_function != CHILD(i2)[0].function()) {
								bool b = false;
								if(mstruct[0].isNumber()) {
									if(CHILD(0).isNumber()) {
										if(mstruct.size() == SIZE - 1 && CHILD(0).number() == -mstruct[0].number()) {
											b = true;
											for(size_t i = 1; i < mstruct.size(); i++) {
												if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) {b = false; break;}
											}
										}
									} else if(mstruct.size() == SIZE && mstruct[0].isMinusOne()) {
										b = true;
										for(size_t i = 1; i < mstruct.size(); i++) {
											if(!mstruct[i].equals(CHILD(i2 >= i ? i - 1 : i))) {b = false; break;}
										}
									}
								} else if(mstruct.size() == SIZE - 2 && CHILD(0).isMinusOne()) {
									b = true;
									for(size_t i = 0; i < mstruct.size(); i++) {
										if(!mstruct[i].equals(CHILD(i2 - 1 >= i ? i + 1 : i + 2))) {b = false; break;}
									}
								}
								if(b) {
									CHILD(i2)[0].setFunctionId(CHILD(i2)[0].function()->id() == FUNCTION_ID_COS ? FUNCTION_ID_SIN : FUNCTION_ID_COS);
									MERGE_APPROX_AND_PREC(mstruct)
									calculateNegate(eo, mparent, index_this);
									return 1;
								}
							}
						} else if(eo.transform_trigonometric_functions && CHILD(i2).isPower() && CHILD(i2)[0].isFunction() && (CHILD(i2)[0].function()->id() == FUNCTION_ID_COSH || CHILD(i2)[0].function()->id() == FUNCTION_ID_SINH) && CHILD(i2)[0].size() == 1 && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isTwo() && mstruct.size() > 0) {
							if(!FUNCTION_PROTECTED(eo, CHILD(i2)[0].function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH)) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(mstruct[i].isPower() && mstruct[i][0].isFunction() && mstruct[i][0].function()->id() == (CHILD(i2)[0].function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH) && mstruct[i][0].size() == 1 && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && CHILD(i2)[0][0] == mstruct[i][0][0]) {
										MathStructure madd1(*this);
										MathStructure madd2(mstruct);
										MathStructure madd;
										madd1.delChild(i2 + 1, true);
										madd2.delChild(i + 1, true);
										if(mstruct[i][0].function()->id() == FUNCTION_ID_SINH) madd = madd1;
										else madd = madd2;
										if(madd1.calculateAdd(madd2, eo)) {
											SET_CHILD_MAP(i2);
											CHILD(0).setFunctionId(FUNCTION_ID_SINH);
											calculateMultiply(madd1, eo);
											EvaluationOptions eo2 = eo;
											eo2.transform_trigonometric_functions = false;
											calculateAdd(madd, eo2);
											return 1;
										}
									}
								}
							}
							if(eo.protected_function != CHILD(i2)[0].function()) {
								if(CHILD(i2)[0].function()->id() == FUNCTION_ID_SINH && mstruct.size() == SIZE - 1) {
									for(size_t i = 0; i < mstruct.size(); i++) {
										if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) break;
										if(i == mstruct.size() - 1) {
											CHILD(i2)[0].setFunctionId(FUNCTION_ID_COSH);
											MERGE_APPROX_AND_PREC(mstruct)
											return 1;
										}
									}
								} else if(CHILD(i2)[0].function()->id() == FUNCTION_ID_COSH) {
									bool b = false;
									if(mstruct[0].isNumber()) {
										if(CHILD(0).isNumber()) {
											if(mstruct.size() == SIZE - 1 && CHILD(0).number() == -mstruct[0].number()) {
												b = true;
												for(size_t i = 1; i < mstruct.size(); i++) {
													if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) {b = false; break;}
												}
											}
										} else if(mstruct.size() == SIZE && mstruct[0].isMinusOne()) {
											b = true;
											for(size_t i = 1; i < mstruct.size(); i++) {
												if(!mstruct[i].equals(CHILD(i2 >= i ? i - 1 : i))) {b = false; break;}
											}
										}
									} else if(mstruct.size() == SIZE - 2 && CHILD(0).isMinusOne()) {
										b = true;
										for(size_t i = 1; i < mstruct.size(); i++) {
											if(!mstruct[i].equals(CHILD(i2 - 1 >= i ? i + 1 : i + 2))) {b = false; break;}
										}
									}
									if(b) {
										CHILD(i2)[0].setFunctionId(FUNCTION_ID_SINH);
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									}
								}
							}
						} else if(CHILD(i2).isFunction()) {
							if(CHILD(i2).function()->id() == FUNCTION_ID_SIGNUM && !FUNCTION_PROTECTED(eo, FUNCTION_ID_SIGNUM) && CHILD(i2).size() == 2 && CHILD(i2)[0].isAddition() && CHILD(i2)[0].size() == 2 && CHILD(i2)[0].representsReal(true)) {
								for(size_t im = 0; im < mstruct.size(); im++) {
									if(mstruct[im] == CHILD(i2)) {
										MathStructure m1(*this), m2(mstruct);
										m1.delChild(i2 + 1, true);
										m2.delChild(im + 1, true);
										if((m1 == CHILD(i2)[0][0] && m2 == CHILD(i2)[0][1]) || (m2 == CHILD(i2)[0][0] && m1 == CHILD(i2)[0][1])) {
											SET_CHILD_MAP(i2)
											setFunctionId(FUNCTION_ID_ABS);
											ERASE(1)
											MERGE_APPROX_AND_PREC(mstruct)
											return 1;
										}
									}
								}
							} else if(CHILD(i2).function()->id() == FUNCTION_ID_ASIN || CHILD(i2).function()->id() == FUNCTION_ID_ACOS) {
								CHILD(i2).setFunctionId(CHILD(i2).function()->id() == FUNCTION_ID_ASIN ? FUNCTION_ID_ACOS : FUNCTION_ID_ASIN);
								if(equals(mstruct)) {
									//asin(x)+acos(x)=pi/2
									delChild(i2 + 1, true);
									switch(eo.parse_options.angle_unit) {
										case ANGLE_UNIT_DEGREES: {calculateMultiply(Number(90, 1, 0), eo); break;}
										case ANGLE_UNIT_GRADIANS: {calculateMultiply(Number(100, 1, 0), eo); break;}
										case ANGLE_UNIT_RADIANS: {calculateMultiply(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo); calculateMultiply(nr_half, eo); break;}
										default: {calculateMultiply(CALCULATOR->getVariableById(VARIABLE_ID_PI), eo); calculateMultiply(nr_half, eo); if(CALCULATOR->getRadUnit()) {calculateMultiply(CALCULATOR->getRadUnit(), eo);} break;}
									}
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								CHILD(i2).setFunctionId(CHILD(i2).function()->id() == FUNCTION_ID_ASIN ? FUNCTION_ID_ACOS : FUNCTION_ID_ASIN);
							} else if(CHILD(i2).function()->id() == FUNCTION_ID_SINH || CHILD(i2).function()->id() == FUNCTION_ID_COSH) {
								CHILD(i2).setFunctionId(CHILD(i2).function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH);
								if(equals(mstruct)) {
									//sinh(x)+cosh(x)=e^x
									MathStructure *mexp = &CHILD(i2)[0];
									mexp->ref();
									delChild(i2 + 1, true);
									MathStructure *mmul = new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_E));
									mmul->raise_nocopy(mexp);
									mmul->calculateRaiseExponent(eo);
									MERGE_APPROX_AND_PREC(mstruct)
									multiply_nocopy(mmul);
									calculateMultiplyLast(eo, true, mparent, index_this);
									return 1;
								}
								CHILD(i2).setFunctionId(CHILD(i2).function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH);
							}
						}
					}

					if(eo.transform_trigonometric_functions) {
						for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
							if(mstruct[i2].isPower() && mstruct[i2][0].isFunction() && (mstruct[i2][0].function()->id() == FUNCTION_ID_COS || mstruct[i2][0].function()->id() == FUNCTION_ID_SIN) && mstruct[i2][0].size() == 1 && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo() && SIZE > 0) {
								if(eo.protected_function != mstruct[i2][0].function()) {
									bool b = false;
									if(CHILD(0).isNumber()) {
										if(mstruct[0].isNumber()) {
											if(mstruct.size() - 1 == SIZE && CHILD(0).number() == -mstruct[0].number()) {
												b = true;
												for(size_t i = 1; i < SIZE; i++) {
													if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) {b = false; break;}
												}
											}
										} else if(mstruct.size() == SIZE && CHILD(0).isMinusOne()) {
											b = true;
											for(size_t i = 1; i < SIZE; i++) {
												if(!CHILD(i).equals(mstruct[i2 >= i ? i - 1 : i])) {b = false; break;}
											}
										}
									} else if(mstruct.size() - 2 == SIZE && mstruct[0].isMinusOne()) {
										b = true;
										for(size_t i = 0; i < SIZE; i++) {
											if(!CHILD(i).equals(mstruct[i2 - 1 >= i ? i + 1 : i + 2])) {b = false; break;}
										}
									}
									if(b) {
										mstruct[i2][0].setFunctionId(mstruct[i2][0].function()->id() == FUNCTION_ID_COS ? FUNCTION_ID_SIN : FUNCTION_ID_COS);
										mstruct.calculateNegate(eo);
										if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
										else set_nocopy(mstruct, true);
										return 1;
									}
								}
							} else if(mstruct[i2].isPower() && mstruct[i2][0].isFunction() && (mstruct[i2][0].function()->id() == FUNCTION_ID_COSH || mstruct[i2][0].function()->id() == FUNCTION_ID_SINH) && mstruct[i2][0].size() == 1 && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo() && SIZE > 0) {
								if(eo.protected_function != mstruct[i2][0].function()) {
									if(mstruct[i2][0].function()->id() == FUNCTION_ID_SINH && mstruct.size() - 1 == SIZE) {
										for(size_t i = 0; i < SIZE; i++) {
											if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) break;
											if(i == SIZE - 1) {
												mstruct[i2][0].setFunctionId(FUNCTION_ID_COSH);
												if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
												else set_nocopy(mstruct, true);
												return 1;
											}
										}
									} else if(mstruct[i2][0].function()->id() == FUNCTION_ID_COSH) {
										bool b = false;
										if(CHILD(0).isNumber()) {
											if(mstruct[0].isNumber()) {
												if(mstruct.size() - 1 == SIZE && CHILD(0).number() == -mstruct[0].number()) {
													b = true;
													for(size_t i = 1; i < SIZE; i++) {
														if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) {b = false; break;}
													}
												}
											} else if(mstruct.size() == SIZE && CHILD(0).isMinusOne()) {
												b = true;
												for(size_t i = 1; i < SIZE; i++) {
													if(!CHILD(i).equals(mstruct[i2 >= i ? i - 1 : i])) {b = false; break;}
												}
											}
										} else if(mstruct.size() - 2 == SIZE && mstruct[0].isMinusOne()) {
											b = true;
											for(size_t i = 1; i < SIZE; i++) {
												if(!CHILD(i).equals(mstruct[i2 - 1 >= i ? i + 1 : i + 2])) {b = false; break;}
											}
										}
										if(b) {
											mstruct[i2][0].setFunctionId(FUNCTION_ID_SINH);
											if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
											else set_nocopy(mstruct, true);
											return 1;
										}
									}
								}
							}
						}
					}

					if(!eo.combine_divisions) break;
					b = true; size_t divs = 0;
					for(; b && i1 < SIZE; i1++) {
						if(CHILD(i1).isPower() && CHILD(i1)[1].hasNegativeSign()) {
							divs++;
							b = false;
							for(; i2 < mstruct.size(); i2++) {
								if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
									if(mstruct[i2] == CHILD(i1)) {
										b = true;
									}
									i2++;
									break;
								}
							}
						}
					}
					if(b && divs > 0) {
						for(; i2 < mstruct.size(); i2++) {
							if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
								b = false;
								break;
							}
						}
					}
					if(b && divs > 0) {
						if(SIZE - divs == 0) {
							if(mstruct.size() - divs == 0) {
								calculateMultiply(nr_two, eo);
							} else if(mstruct.size() - divs == 1) {
								PREPEND(m_one);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(0).add_nocopy(&mstruct[i], true);
										CHILD(0).calculateAddLast(eo, true, this, 0);
										break;
									}
								}
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							} else {
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								PREPEND(m_one);
								mstruct.ref();
								CHILD(0).add_nocopy(&mstruct, true);
								CHILD(0).calculateAddLast(eo, true, this, 0);
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							}
						} else if(SIZE - divs == 1) {
							size_t index = 0;
							for(; index < SIZE; index++) {
								if(!CHILD(index).isPower() || !CHILD(index)[1].hasNegativeSign()) {
									break;
								}
							}
							if(mstruct.size() - divs == 0) {
								if(IS_REAL(CHILD(index))) {
									CHILD(index).number()++;
								} else {
									CHILD(index).calculateAdd(m_one, eo, this, index);
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(index).add_nocopy(&mstruct[i], true);
										CHILD(index).calculateAddLast(eo, true, this, index);
										break;
									}
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else {
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								mstruct.ref();
								CHILD(index).add_nocopy(&mstruct, true);
								CHILD(index).calculateAddLast(eo, true, this, index);
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							}
						} else {
							for(size_t i = 0; i < SIZE;) {
								if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
									ERASE(i);
								} else {
									i++;
								}
							}
							if(mstruct.size() - divs == 0) {
								calculateAdd(m_one, eo);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										add_nocopy(&mstruct[i], true);
										calculateAddLast(eo);
										break;
									}
								}
							} else {
								MathStructure *mstruct2 = new MathStructure();
								mstruct2->setType(STRUCT_MULTIPLICATION);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										mstruct2->addChild_nocopy(&mstruct[i]);
									}
								}
								add_nocopy(mstruct2, true);
								calculateAddLast(eo, true, mparent, index_this);
							}
							for(size_t i = 0; i < mstruct.size(); i++) {
								if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
									mstruct[i].ref();
									multiply_nocopy(&mstruct[i], true);
									calculateMultiplyLast(eo);
								}
							}
						}
						return 1;
					}

					break;
				}
				case STRUCT_POWER: {
					if(eo.combine_divisions && mstruct[1].hasNegativeSign()) {
						bool b = false;
						size_t index = 0;
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
								if(b) {
									b = false;
									break;
								}
								if(mstruct == CHILD(i)) {
									index = i;
									b = true;
								}
								if(!b) break;
							}
						}
						if(b) {
							if(SIZE == 2) {
								if(index == 0) setToChild(2, true);
								else setToChild(1, true);
							} else {
								ERASE(index);
							}
							calculateAdd(m_one, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct, false);
							calculateMultiplyLast(eo, true, mparent, index_this);
							return 1;
						}
					}
					if(eo.transform_trigonometric_functions && SIZE == 2 && CHILD(0).isNumber() && mstruct[0].isFunction()) {
						if((mstruct[0].function()->id() == FUNCTION_ID_COS || mstruct[0].function()->id() == FUNCTION_ID_SIN) && !FUNCTION_PROTECTED(eo, mstruct[0].function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo()) {
							if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function()->id() == (mstruct[0].function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN) && CHILD(1)[0].size() == 1 && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && mstruct[0][0] == CHILD(1)[0][0]) {
								if(CHILD(0).calculateSubtract(m_one, eo)) {
									add(m_one);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
							}
						} else if((mstruct[0].function()->id() == FUNCTION_ID_COSH || mstruct[0].function()->id() == FUNCTION_ID_SINH) && !FUNCTION_PROTECTED(eo, mstruct[0].function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo()) {
							if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function()->id() == (mstruct[0].function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH) && CHILD(1)[0].size() == 1 && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && mstruct[0][0] == CHILD(1)[0][0]) {
								if(CHILD(0).calculateAdd(m_one, eo)) {
									add(CHILD(1)[0].function()->id() == FUNCTION_ID_SINH ? m_one : m_minus_one);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
							}
						}
					}
				}
				default: {
					if(SIZE == 2 && CHILD(0).isNumber() && CHILD(1) == mstruct) {
						CHILD(0).number()++;
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
						return 1;
					}
					if(eo.transform_trigonometric_functions && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function() != eo.protected_function && CHILD(1)[1] == nr_two && CHILD(1)[0].size() == 1) {
						if(mstruct.isNumber() && CHILD(0).isNumber()) {
							if(CHILD(1)[0].function()->id() == FUNCTION_ID_SIN && mstruct.number() == -CHILD(0).number()) {
								CHILD(1)[0].setFunctionId(FUNCTION_ID_COS);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(1)[0].function()->id() == FUNCTION_ID_COS && mstruct.number() == -CHILD(0).number()) {
								CHILD(1)[0].setFunctionId(FUNCTION_ID_SIN);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(1)[0].function()->id() == FUNCTION_ID_COSH && mstruct.number() == -CHILD(0).number()) {
								MERGE_APPROX_AND_PREC(mstruct)
								CHILD(1)[0].setFunctionId(FUNCTION_ID_SINH);
								return 1;
							} else if(CHILD(1)[0].function()->id() == FUNCTION_ID_SINH && mstruct == CHILD(0)) {
								CHILD(1)[0].setFunctionId(FUNCTION_ID_COSH);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					} else if(eo.transform_trigonometric_functions && SIZE == 3 && CHILD(0).isMinusOne()) {
						size_t i = 0;
						if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && (CHILD(1)[0].function()->id() == FUNCTION_ID_SIN || CHILD(1)[0].function()->id() == FUNCTION_ID_COS || CHILD(1)[0].function()->id() == FUNCTION_ID_COSH) && CHILD(1)[0].function() != eo.protected_function && CHILD(1)[1] == nr_two && CHILD(1)[0].size() == 1 && CHILD(2) == mstruct) i = 1;
						if(CHILD(2).isPower() && CHILD(2)[0].isFunction() && (CHILD(2)[0].function()->id() == FUNCTION_ID_SIN || CHILD(2)[0].function()->id() == FUNCTION_ID_COS || CHILD(2)[0].function()->id() == FUNCTION_ID_COSH) && CHILD(2)[0].function() != eo.protected_function && CHILD(2)[1] == nr_two && CHILD(2)[0].size() == 1 && CHILD(1) == mstruct) i = 2;
						if(i > 0) {
							if(CHILD(i)[0].function()->id() == FUNCTION_ID_SIN) {
								CHILD(i)[0].setFunctionId(FUNCTION_ID_COS);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(i)[0].function()->id() == FUNCTION_ID_COS) {
								CHILD(i)[0].setFunctionId(FUNCTION_ID_SIN);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(i)[0].function()->id() == FUNCTION_ID_COSH) {
								MERGE_APPROX_AND_PREC(mstruct)
								CHILD(i)[0].setFunctionId(FUNCTION_ID_SINH);
								return 1;
							}
						}
					}
					if(mstruct.isDateTime() || (mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && !FUNCTION_PROTECTED(eo, FUNCTION_ID_SIGNUM))) {
						return 0;
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isFunction() && (CHILD(0).function()->id() == FUNCTION_ID_COS || CHILD(0).function()->id() == FUNCTION_ID_SIN) && !FUNCTION_PROTECTED(eo, CHILD(0).function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN) && CHILD(0).size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isTwo()) {
				if(eo.transform_trigonometric_functions && mstruct.isMinusOne()) {
					CHILD(0).setFunctionId(CHILD(0).function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN);
					negate();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function()->id() == (CHILD(0).function()->id() == FUNCTION_ID_SIN ? FUNCTION_ID_COS : FUNCTION_ID_SIN) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo() && CHILD(0)[0] == mstruct[0][0]) {
					// cos(x)^2+sin(x)^2=1
					set(1, 1, 0, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(eo.transform_trigonometric_functions && CHILD(0).isFunction() && (CHILD(0).function()->id() == FUNCTION_ID_COSH || CHILD(0).function()->id() == FUNCTION_ID_SINH) && !FUNCTION_PROTECTED(eo, CHILD(0).function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH) && CHILD(0).size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isTwo()) {
				// cosh(x)^2=sinh(x)^2+1
				if(CHILD(0).function()->id() == FUNCTION_ID_SINH && mstruct.isOne()) {
					CHILD(0).setFunctionId(FUNCTION_ID_COSH);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(CHILD(0).function()->id() == FUNCTION_ID_COSH && mstruct.isMinusOne()) {
					CHILD(0).setFunctionId(FUNCTION_ID_SINH);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function()->id() == (CHILD(0).function()->id() == FUNCTION_ID_SINH ? FUNCTION_ID_COSH : FUNCTION_ID_SINH) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo() && CHILD(0)[0] == mstruct[0][0]) {
					if(CHILD(0).function()->id() == FUNCTION_ID_SINH) {
						multiply(nr_two);
						add(m_one);
					} else {
						CHILD(0).setFunctionId(FUNCTION_ID_SINH);
						multiply(nr_two);
						add(m_minus_one);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			goto default_addition_merge;
		}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_SIGNUM && mstruct.isMultiplication() && !FUNCTION_PROTECTED(eo, FUNCTION_ID_SIGNUM)) {
				if(SIZE == 2 && CHILD(0).isAddition() && CHILD(0).size() == 2 && (CHILD(0)[0].isOne() || CHILD(0)[1].isOne()) && CHILD(0).representsReal(true)) {
					for(size_t im = 0; im < mstruct.size(); im++) {
						if(mstruct[im] == *this) {
							MathStructure m2(mstruct);
							m2.delChild(im + 1, true);
							if((CHILD(0)[0].isOne() && m2 == CHILD(0)[1]) || (CHILD(0)[1].isOne() && m2 == CHILD(0)[0])) {
								setFunctionId(FUNCTION_ID_ABS);
								ERASE(1)
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					}
				}
			} else if(mstruct.isFunction() && ((o_function->id() == FUNCTION_ID_ASIN && mstruct.function()->id() == FUNCTION_ID_ACOS) || (o_function->id() == FUNCTION_ID_ACOS && mstruct.function()->id() == FUNCTION_ID_ASIN)) && !FUNCTION_PROTECTED(eo, FUNCTION_ID_ACOS) && !FUNCTION_PROTECTED(eo, FUNCTION_ID_ASIN) && SIZE == 1 && mstruct.size() == 1 && CHILD(0) == mstruct[0]) {
				//asin(x)+acos(x)=pi/2
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {set(90, 1, 0, true); break;}
					case ANGLE_UNIT_GRADIANS: {set(100, 1, 0, true); break;}
					case ANGLE_UNIT_RADIANS: {set(CALCULATOR->getVariableById(VARIABLE_ID_PI), true); multiply(nr_half); calculatesub(eo, eo, true); break;}
					default: {set(CALCULATOR->getVariableById(VARIABLE_ID_PI), true); multiply(nr_half); if(CALCULATOR->getRadUnit()) {multiply(CALCULATOR->getRadUnit(), true);} calculatesub(eo, eo, true); break;}
				}
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(mstruct.isFunction() && ((o_function->id() == FUNCTION_ID_SINH && mstruct.function()->id() == FUNCTION_ID_COSH) || (o_function->id() == FUNCTION_ID_COSH && mstruct.function()->id() == FUNCTION_ID_SINH)) && !FUNCTION_PROTECTED(eo, FUNCTION_ID_COSH) && !FUNCTION_PROTECTED(eo, FUNCTION_ID_SINH) && SIZE == 1 && mstruct.size() == 1 && CHILD(0) == mstruct[0]) {
				//sinh(x)+cosh(x)=e^x
				MathStructure *mexp = &CHILD(0);
				mexp->ref();
				set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
				calculatesub(eo, eo, true);
				raise_nocopy(mexp);
				MERGE_APPROX_AND_PREC(mstruct)
				calculateRaiseExponent(eo, mparent, index_this);
				return 1;
			} else if(mstruct.isFunction() && o_function->id() == FUNCTION_ID_STRIP_UNITS && mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS && mstruct.size() == 1 && SIZE == 1) {
				mstruct[0].ref();
				CHILD(0).add_nocopy(&mstruct[0]);
				return 1;
			}
			goto default_addition_merge;
		}
		case STRUCT_DATETIME: {
			if(mstruct.isDateTime()) {
				if(o_datetime->add(*mstruct.datetime())) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne() && mstruct[1].isDateTime() && (CALCULATOR->getUnitById(UNIT_ID_SECOND) || CALCULATOR->getUnitById(UNIT_ID_DAY))) {
				if(CALCULATOR->getUnitById(UNIT_ID_DAY) && !mstruct[1].datetime()->timeIsSet() && !o_datetime->timeIsSet()) {
					Number ndays = mstruct[1].datetime()->daysTo(*o_datetime);
					set(ndays, true);
					multiply(CALCULATOR->getUnitById(UNIT_ID_DAY));
				} else {
					Number nsecs = mstruct[1].datetime()->secondsTo(*o_datetime, true);
					set(nsecs, true);
					multiply(CALCULATOR->getUnitById(UNIT_ID_SECOND), true);
				}
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(CALCULATOR->getUnitById(UNIT_ID_SECOND) && ((mstruct.isUnit() && mstruct.unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_SECOND) && mstruct.unit()->baseExponent() == 1 && !mstruct.unit()->hasNonlinearRelationTo(CALCULATOR->getUnitById(UNIT_ID_SECOND))) || (mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[0].number().isReal() && !mstruct[0].number().isInterval() && mstruct[1].isUnit() && mstruct[1].unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_SECOND) && mstruct[1].unit()->baseExponent() == 1 && !mstruct[1].unit()->hasNonlinearRelationTo(CALCULATOR->getUnitById(UNIT_ID_SECOND))))) {
				MathStructure mmul(1, 1, 0);
				Unit *u;
				if(mstruct.isMultiplication()) {
					mmul = mstruct[0];
					u = mstruct[1].unit();
				} else {
					u = mstruct.unit();
				}
				if(CALCULATOR->getUnitById(UNIT_ID_MONTH) && u != CALCULATOR->getUnitById(UNIT_ID_YEAR) && (u == CALCULATOR->getUnitById(UNIT_ID_MONTH) || u->isChildOf(CALCULATOR->getUnitById(UNIT_ID_MONTH)))) {
					if(u != CALCULATOR->getUnitById(UNIT_ID_MONTH)) {
						CALCULATOR->getUnitById(UNIT_ID_MONTH)->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addMonths(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->getUnitById(UNIT_ID_YEAR) && (u == CALCULATOR->getUnitById(UNIT_ID_YEAR) || u->isChildOf(CALCULATOR->getUnitById(UNIT_ID_YEAR)))) {
					if(u != CALCULATOR->getUnitById(UNIT_ID_YEAR)) {
						CALCULATOR->getUnitById(UNIT_ID_YEAR)->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addYears(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->getUnitById(UNIT_ID_DAY) && (u == CALCULATOR->getUnitById(UNIT_ID_DAY) || u->isChildOf(CALCULATOR->getUnitById(UNIT_ID_DAY)))) {
					if(u != CALCULATOR->getUnitById(UNIT_ID_DAY)) {
						CALCULATOR->getUnitById(UNIT_ID_DAY)->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addDays(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->getUnitById(UNIT_ID_HOUR) && (u == CALCULATOR->getUnitById(UNIT_ID_HOUR) || u->isChildOf(CALCULATOR->getUnitById(UNIT_ID_HOUR)))) {
					if(u != CALCULATOR->getUnitById(UNIT_ID_HOUR)) {
						CALCULATOR->getUnitById(UNIT_ID_HOUR)->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addHours(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->getUnitById(UNIT_ID_MINUTE) && (u == CALCULATOR->getUnitById(UNIT_ID_MINUTE) || u->isChildOf(CALCULATOR->getUnitById(UNIT_ID_MINUTE)))) {
					if(u != CALCULATOR->getUnitById(UNIT_ID_MINUTE)) {
						CALCULATOR->getUnitById(UNIT_ID_MINUTE)->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addMinutes(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else {
					MathStructure mmulb(mmul);
					if(u != CALCULATOR->getUnitById(UNIT_ID_SECOND)) {
						u->convertToBaseUnit(mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addSeconds(mmul.number(), true)) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				}
			}
		}
		default: {
			default_addition_merge:
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_DATETIME: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				default: {
					if(equals(mstruct)) {
						multiply_nocopy(new MathStructure(2, 1, 0), true);
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyLast(eo, true, mparent, index_this);
						return 1;
					}
				}
			}
		}
	}
	return -1;
}

bool reducable(const MathStructure &mnum, const MathStructure &mden, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			bool reduce = true;
			for(size_t i = 0; i < mden.size() && reduce; i++) {
				switch(mden[i].type()) {
					case STRUCT_MULTIPLICATION: {
						reduce = false;
						for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
							if(mnum == mden[i][i2]) {
								reduce = true;
								if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1, 0);
								break;
							} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum == mden[i][i2][0]) {
								if(!mden[i][i2][1].number().isPositive()) {
									break;
								}
								if(mden[i][i2][1].number().isLessThan(nr)) nr = mden[i][i2][1].number();
								reduce = true;
								break;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum == mden[i][0]) {
							if(!mden[i][1].number().isPositive()) {
								reduce = false;
								break;
							}
							if(mden[i][1].number().isLessThan(nr)) nr = mden[i][1].number();
							break;
						}
					}
					default: {
						if(mnum != mden[i]) {
							reduce = false;
							break;
						}
						if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1, 0);
					}
				}
			}
			return reduce;
		}
	}
	return false;
}
void reduce(const MathStructure &mnum, MathStructure &mden, Number &nr, const EvaluationOptions &eo) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			for(size_t i = 0; i < mden.size(); i++) {
				switch(mden[i].type()) {
					case STRUCT_MULTIPLICATION: {
						for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
							if(mden[i][i2] == mnum) {
								if(!nr.isOne()) {
									MathStructure *mexp = new MathStructure(1, 1, 0);
									mexp->number() -= nr;
									mden[i][i2].raise_nocopy(mexp);
									mden[i][i2].calculateRaiseExponent(eo);
									if(mden[i][i2].isOne() && mden[i].size() > 1) {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								} else {
									if(mden[i].size() == 1) {
										mden[i].set(m_one);
									} else {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								}
								break;
							} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum.equals(mden[i][i2][0])) {
								mden[i][i2][1].number() -= nr;
								if(mden[i][i2][1].number().isOne()) {
									mden[i][i2].setToChild(1, true, &mden[i], i2 + 1);
								} else {
									mden[i][i2].calculateRaiseExponent(eo);
									if(mden[i][i2].isOne() && mden[i].size() > 1) {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								}
								break;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum.equals(mden[i][0])) {
							mden[i][1].number() -= nr;
							if(mden[i][1].number().isOne()) {
								mden[i].setToChild(1, true, &mden, i + 1);
							} else {
								mden[i].calculateRaiseExponent(eo, &mden, i);
							}
							break;
						}
					}
					default: {
						if(!nr.isOne()) {
							MathStructure *mexp = new MathStructure(1, 1, 0);
							mexp->number() -= nr;
							mden[i].raise_nocopy(mexp);
							mden[i].calculateRaiseExponent(eo, &mden, 1);
						} else {
							mden[i].set(m_one);
						}
					}
				}
			}
			mden.calculatesub(eo, eo, false);
		}
	}
}

bool addablePower(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct[0].representsNonNegative(true)) return true;
	if(mstruct[1].representsInteger()) return true;
	//return eo.allow_complex && mstruct[0].representsNegative(true) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven();
	return eo.allow_complex && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven();
}

int MathStructure::merge_multiplication(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed, bool do_append) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.multiply(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	} else if(isOne()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(o_number.isMinusInfinity(false)) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(o_number.isPlusInfinity(false)) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mtest(mstruct);
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				mtest.calculateFunctions(eo2);
				mtest.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
			if(o_number.isMinusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				} else if(mtest.representsNegative(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				} else if(mtest.representsNegative(false)) {
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(mstruct.number().isMinusInfinity(false)) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.number().isPlusInfinity(false)) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mtest(*this);
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				mtest.calculateFunctions(eo2);
				mtest.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
			if(mstruct.number().isMinusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					clear(true);
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.representsNegative(false)) {
					clear(true);
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(mstruct.number().isPlusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					clear(true);
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.representsNegative(false)) {
					clear(true);
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	}

	if(representsUndefined() || mstruct.representsUndefined()) return -1;

	// x/(x^2+x)=1/(x+1)
	const MathStructure *mnum = NULL, *mden = NULL;
	bool b_nonzero = false;
	if(eo.reduce_divisions) {
		if(!isNumber() && mstruct.isPower() && mstruct[0].isAddition() && mstruct[0].size() > 1 && mstruct[1].isNumber() && mstruct[1].number().isMinusOne()) {
			if((!isPower() || !CHILD(1).hasNegativeSign()) && representsNumber() && mstruct[0].representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true)) || mstruct[0].representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true))) {
					mnum = this;
					mden = &mstruct[0];
				}
			}
		} else if(!mstruct.isNumber() && isPower() && CHILD(0).isAddition() && CHILD(0).size() > 1 && CHILD(1).isNumber() && CHILD(1).number().isMinusOne()) {
			if((!mstruct.isPower() || !mstruct[1].hasNegativeSign()) && mstruct.representsNumber() && CHILD(0).representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) || CHILD(0).representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))) {
					mnum = &mstruct;
					mden = &CHILD(0);
				}
			}
		}
	}

	if(mnum && mden && eo.reduce_divisions) {
		switch(mnum->type()) {
			case STRUCT_ADDITION: {
				break;
			}
			case STRUCT_MULTIPLICATION: {
				Number nr;
				vector<Number> nrs;
				vector<size_t> reducables;
				for(size_t i = 0; i < mnum->size(); i++) {
					switch((*mnum)[i].type()) {
						case STRUCT_ADDITION: {break;}
						case STRUCT_POWER: {
							if((*mnum)[i][1].isNumber() && (*mnum)[i][1].number().isReal()) {
								if((*mnum)[i][1].number().isPositive()) {
									nr.set((*mnum)[i][1].number());
									if(reducable((*mnum)[i][0], *mden, nr)) {
										nrs.push_back(nr);
										reducables.push_back(i);
									}
								}
								break;
							}
						}
						default: {
							nr.set(1, 1, 0);
							if(reducable((*mnum)[i], *mden, nr)) {
								nrs.push_back(nr);
								reducables.push_back(i);
							}
						}
					}
				}
				if(reducables.size() > 0) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						mstruct.ref();
						transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
					} else {
						transform(STRUCT_MULTIPLICATION);
						PREPEND_REF(&mstruct);
					}
					size_t i_erased = 0;
					for(size_t i = 0; i < reducables.size(); i++) {
						switch(CHILD(0)[reducables[i] - i_erased].type()) {
							case STRUCT_POWER: {
								if(CHILD(0)[reducables[i] - i_erased][1].isNumber() && CHILD(0)[reducables[i] - i_erased][1].number().isReal()) {
									reduce(CHILD(0)[reducables[i] - i_erased][0], CHILD(1)[0], nrs[i], eo);
									if(nrs[i] == CHILD(0)[reducables[i] - i_erased][1].number()) {
										CHILD(0).delChild(reducables[i] - i_erased + 1);
										i_erased++;
									} else {
										CHILD(0)[reducables[i] - i_erased][1].number() -= nrs[i];
										if(CHILD(0)[reducables[i] - i_erased][1].number().isOne()) {
											CHILD(0)[reducables[i] - i_erased].setToChild(1, true, &CHILD(0), reducables[i] - i_erased + 1);
										} else {
											CHILD(0)[reducables[i] - i_erased].calculateRaiseExponent(eo);
										}
										CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true);
									}
									break;
								}
							}
							default: {
								reduce(CHILD(0)[reducables[i] - i_erased], CHILD(1)[0], nrs[i], eo);
								if(nrs[i].isOne()) {
									CHILD(0).delChild(reducables[i] - i_erased + 1);
									i_erased++;
								} else {
									MathStructure mexp(1, 1);
									mexp.number() -= nrs[i];
									CHILD(0)[reducables[i] - i_erased].calculateRaise(mexp, eo);
									CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true);
								}
							}
						}
					}
					if(CHILD(0).size() == 0) {
						setToChild(2, true, mparent, index_this + 1);
					} else if(CHILD(0).size() == 1) {
						CHILD(0).setToChild(1, true, this, 1);
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					} else {
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					}
					return 1;
				}
				break;
			}
			case STRUCT_POWER: {
				if((*mnum)[1].isNumber() && (*mnum)[1].number().isReal()) {
					if((*mnum)[1].number().isPositive()) {
						Number nr((*mnum)[1].number());
						if(reducable((*mnum)[0], *mden, nr)) {
							if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
							if(nr != (*mnum)[1].number()) {
								MathStructure mnum2((*mnum)[0]);
								if(mnum == this) {
									CHILD(1).number() -= nr;
									if(CHILD(1).number().isOne()) {
										set(mnum2);
									} else {
										calculateRaiseExponent(eo);
									}
									mstruct.ref();
									transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
									reduce(mnum2, CHILD(1)[0], nr, eo);
									calculateMultiplyLast(eo);
								} else {
									transform(STRUCT_MULTIPLICATION);
									PREPEND(mstruct);
									CHILD(0)[1].number() -= nr;
									if(CHILD(0)[1].number().isOne()) {
										CHILD(0) = mnum2;
									} else {
										CHILD(0).calculateRaiseExponent(eo);
									}
									reduce(mnum2, CHILD(1)[0], nr, eo);
									calculateMultiplyIndex(0, eo);
								}
							} else {
								if(mnum == this) {
									MathStructure mnum2((*mnum)[0]);
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
										reduce(mnum2, (*mparent)[index_this][0], nr, eo);
									} else {
										set_nocopy(mstruct, true);
										reduce(mnum2, CHILD(0), nr, eo);
									}
								} else {
									reduce((*mnum)[0], CHILD(0), nr, eo);
								}
							}
							return 1;
						}
					}
					break;
				}
			}
			default: {
				Number nr(1, 1);
				if(reducable(*mnum, *mden, nr)) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						MathStructure mnum2(*mnum);
						if(!nr.isOne()) {
							reduce(*mnum, mstruct[0], nr, eo);
							mstruct.calculateRaiseExponent(eo);
							nr.negate();
							nr++;
							calculateRaise(nr, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						} else if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
							reduce(mnum2, (*mparent)[index_this][0], nr, eo);
							(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
						} else {
							set_nocopy(mstruct, true);
							reduce(mnum2, CHILD(0), nr, eo);
							calculateRaiseExponent(eo, mparent, index_this);
						}
					} else {
						reduce(*mnum, CHILD(0), nr, eo);
						if(!nr.isOne()) {
							calculateRaiseExponent(eo);
							nr.negate();
							nr++;
							mstruct.calculateRaise(nr, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						} else {
							calculateRaiseExponent(eo, mparent, index_this);
						}
					}
					return 1;
				}
			}
		}
	}

	if(mstruct.isFunction() && eo.protected_function != mstruct.function()) {
		if(((mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_SIGNUM && mstruct[0].size() == 2) || (mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2 && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ABS && mstruct[0].size() == 1)) && (equals(mstruct[0][0]) || (isFunction() && o_function->id() == FUNCTION_ID_ABS && SIZE == 1 && CHILD(0) == mstruct[0][0]) || (isPower() && CHILD(0) == mstruct[0][0]) || (isPower() && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1 && CHILD(0)[0] == mstruct[0][0]))) {
				// sgn(abs(x))*x^y=x^y
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
		} else if(mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2) {
			if(equals(mstruct[0]) && representsReal(true)) {
				// sgn(x)*x=abs(x)
				transformById(FUNCTION_ID_ABS);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(isPower() && CHILD(1).representsOdd() && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
				//sgn(x)*x^3=abs(x)^3
				CHILD(0).transformById(FUNCTION_ID_ABS);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct)) {
			if(equals(mstruct[0]) && mstruct[0].representsReal(true) && mstruct[1].number().isOdd()) {
				// root(x, 3)*x=abs(x)^(1/3)*x
				mstruct[0].transformById(FUNCTION_ID_ABS);
				mstruct[1].number().recip();
				mstruct.setType(STRUCT_POWER);
				transform(STRUCT_FUNCTION);
				setFunctionId(FUNCTION_ID_ABS);
				mstruct.ref();
				multiply_nocopy(&mstruct);
				calculateMultiplyLast(eo);
				return 1;
			} else if(isPower() && CHILD(1).representsOdd() && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && mstruct[1].number().isOdd()) {
				// root(x, 3)*x^3=abs(x)^(1/3)*x^3
				mstruct[0].transformById(FUNCTION_ID_ABS);
				mstruct[1].number().recip();
				mstruct.setType(STRUCT_POWER);
				CHILD(0).transformById(FUNCTION_ID_ABS);
				mstruct.ref();
				multiply_nocopy(&mstruct);
				calculateMultiplyLast(eo);
				return 1;
			} else if(isPower() && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1 && CHILD(0)[0].equals(mstruct[0]) && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().isFraction() && CHILD(1).number().denominator() == mstruct[1].number() && CHILD(0).representsReal(true) && CHILD(1).number().numerator() == mstruct[1].number() - 1) {
				// root(x, 3)*abs(x)^(2/3)=x
				SET_CHILD_MAP(0)
				SET_CHILD_MAP(0)
				return 1;
			}
		} else if(eo.transform_trigonometric_functions && mstruct.function()->id() == FUNCTION_ID_SINC && mstruct.size() == 1 && equals(mstruct[0])) {
			// sinc(x)*x=sin(x)
			calculateMultiply(CALCULATOR->getRadUnit(), eo);
			transformById(FUNCTION_ID_SIN);
			if(eo.calculate_functions) calculateFunctions(eo, false);
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		}
	}
	if(isZero()) {
		if(mstruct.isFunction()) {
			if((mstruct.function()->id() == FUNCTION_ID_LOG || mstruct.function()->id() == FUNCTION_ID_EI) && mstruct.size() == 1) {
				if(mstruct[0].representsNonZero() || warn_about_assumed_not_value(mstruct[0], m_zero, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(mstruct.function()->id() == FUNCTION_ID_LI && mstruct.size() == 1) {
				if(mstruct.representsNumber(true) || warn_about_assumed_not_value(mstruct[0], m_one, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			}
		} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[1].representsNumber()) {
			if((mstruct[0].function()->id() == FUNCTION_ID_LOG || mstruct[0].function()->id() == FUNCTION_ID_EI) && mstruct[0].size() == 1) {
				if(mstruct[0][0].representsNonZero() || warn_about_assumed_not_value(mstruct[0][0], m_zero, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(mstruct[0].function()->id() == FUNCTION_ID_LI && mstruct[0].size() == 1) {
				if(mstruct[0].representsNumber(true) || warn_about_assumed_not_value(mstruct[0][0], m_one, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			}
		}
	}

	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					return 0;
				}
				case STRUCT_VECTOR: {
					if(isMatrix() && mstruct.isMatrix()) {
						if(CHILD(0).size() != mstruct.size()) {
							CALCULATOR->error(true, _("The second matrix must have as many rows (was %s) as the first has columns (was %s) for matrix multiplication."), i2s(mstruct.size()).c_str(), i2s(CHILD(0).size()).c_str(), NULL);
							return -1;
						}
						MathStructure msave(*this);
						size_t rows = SIZE;
						clearMatrix(true);
						resizeMatrix(rows, mstruct[0].size(), m_zero);
						MathStructure mtmp;
						for(size_t index_r = 0; index_r < SIZE; index_r++) {
							for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
								for(size_t index = 0; index < msave[0].size(); index++) {
									mtmp = msave[index_r][index];
									mtmp.calculateMultiply(mstruct[index][index_c], eo);
									CHILD(index_r)[index_c].calculateAdd(mtmp, eo, &CHILD(index_r), index_c);
								}
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(isMatrix() && mstruct.isVector()) {
						if(SIZE != mstruct.size() || CHILD(0).size() != 1) {
							CALCULATOR->error(true, _("The second matrix must have as many rows (was %s) as the first has columns (was %s) for matrix multiplication."), i2s(1).c_str(), i2s(CHILD(0).size()).c_str(), NULL);
							return -1;
						}
						MathStructure msave(*this);
						size_t rows = SIZE;
						clearMatrix(true);
						resizeMatrix(rows, mstruct.size(), m_zero);
						MathStructure mtmp;
						for(size_t index_r = 0; index_r < SIZE; index_r++) {
							for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
								CHILD(index_r)[index_c].set(msave[index_r][0]);
								CHILD(index_r)[index_c].calculateMultiply(mstruct[index_c], eo);
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else {
						if(SIZE == mstruct.size()) {
							for(size_t i = 0; i < SIZE; i++) {
								mstruct[i].ref();
								CHILD(i).multiply_nocopy(&mstruct[i], true);
								CHILD(i).calculateMultiplyLast(eo, true);
							}
							m_type = STRUCT_ADDITION;
							MERGE_APPROX_AND_PREC(mstruct)
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						}
					}
					return -1;
				}
				default: {
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateMultiply(mstruct, eo);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
		}
		case STRUCT_ADDITION: {
			if(eo.expand != 0 && containsType(STRUCT_DATETIME, false, true, false) > 0) return -1;
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					if(eo.expand != 0 && SIZE < 1000 && mstruct.size() < 1000 && (SIZE * mstruct.size() < (eo.expand == -1 ? 50 : 500))) {

						if(eo.expand > -2 || (!containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0)) || (representsNonNegative(true) && mstruct.representsNonNegative(true))) {
							MathStructure msave(*this);
							CLEAR;
							for(size_t i = 0; i < mstruct.size(); i++) {
								if(CALCULATOR->aborted()) {
									set(msave);
									return -1;
								}
								APPEND(msave);
								mstruct[i].ref();
								LAST.multiply_nocopy(&mstruct[i], true);
								if(reversed) {
									LAST.swapChildren(1, LAST.size());
									LAST.calculateMultiplyIndex(0, eo, true, this, SIZE - 1);
								} else {
									LAST.calculateMultiplyLast(eo, true, this, SIZE - 1);
								}
							}
							MERGE_APPROX_AND_PREC(mstruct)
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(eo.expand <= -2 && (!mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) || representsNonNegative(true))) {
							for(size_t i = 0; i < SIZE; i++) {
								CHILD(i).calculateMultiply(mstruct, eo, this, i);
							}
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(eo.expand <= -2 && (!containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) || mstruct.representsNonNegative(true))) {
							return 0;
						}
					}
					if(equals(mstruct)) {
						raise_nocopy(new MathStructure(2, 1, 0));
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					break;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber() && *this == mstruct[0]) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true))
						|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne())
						|| representsNonZero(true)
						|| mstruct[1].representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
							if(mparent) {
								mparent->swapChildren(index_this + 1, index_mstruct + 1);
								(*mparent)[index_this][1].number()++;
								(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
							} else {
								set_nocopy(mstruct, true);
								CHILD(1).number()++;
								calculateRaiseExponent(eo, mparent, index_this);
							}
							return 1;
						}
					}
					if(eo.expand == 0 && mstruct[0].isAddition()) return -1;
					if(eo.combine_divisions && mstruct[1].hasNegativeSign()) {
						int ret;
						vector<bool> merged;
						merged.resize(SIZE, false);
						size_t merges = 0;
						MathStructure *mstruct2 = new MathStructure(mstruct);
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isOne()) ret = -1;
							else ret = CHILD(i).merge_multiplication(*mstruct2, eo, NULL, 0, 0, false, false);
							if(ret == 0) {
								ret = mstruct2->merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
								if(ret >= 1) {
									mstruct2->ref();
									setChild_nocopy(mstruct2, i + 1);
								}
							}
							if(ret >= 1) {
								mstruct2->unref();
								if(i + 1 != SIZE) mstruct2 = new MathStructure(mstruct);
								merged[i] = true;
								merges++;
							} else {
								if(i + 1 == SIZE) mstruct2->unref();
								merged[i] = false;
							}
						}
						if(merges == 0) {
							return -1;
						} else if(merges == SIZE) {
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(merges == SIZE - 1) {
							for(size_t i = 0; i < SIZE; i++) {
								if(!merged[i]) {
									mstruct.ref();
									CHILD(i).multiply_nocopy(&mstruct, true);
									break;
								}
							}
							calculatesub(eo, eo, false, mparent, index_this);
						} else {
							MathStructure *mdiv = new MathStructure();
							merges = 0;
							for(size_t i = 0; i - merges < SIZE; i++) {
								if(!merged[i]) {
									CHILD(i - merges).ref();
									if(merges > 0) {
										(*mdiv)[0].add_nocopy(&CHILD(i - merges), merges > 1);
									} else {
										mdiv->multiply(mstruct);
										mdiv->setChild_nocopy(&CHILD(i - merges), 1);
									}
									ERASE(i - merges);
									merges++;
								}
							}
							add_nocopy(mdiv, true);
							calculatesub(eo, eo, false);
						}
						return 1;
					}
					if(eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true))) return -1;
				}
				case STRUCT_MULTIPLICATION: {
					if(do_append && (eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true)))) {
						transform(STRUCT_MULTIPLICATION);
						for(size_t i = 0; i < mstruct.size(); i++) {
							APPEND_REF(&mstruct[i]);
						}
						return 1;
					}
				}
				default: {
					if(eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true))) return -1;
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).multiply(mstruct, true);
						if(reversed) {
							CHILD(i).swapChildren(1, CHILD(i).size());
							CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
						} else {
							CHILD(i).calculateMultiplyLast(eo, true, this, i);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
			return -1;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {
					if(eo.expand == 0 || containsType(STRUCT_DATETIME, false, true, false) > 0) {
						if(!do_append) return -1;
						APPEND_REF(&mstruct);
						return 1;
					}
					return 0;
				}
				case STRUCT_MULTIPLICATION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							PREPEND_REF(&mstruct[i]);
							calculateMultiplyIndex(0, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateMultiplyLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber()) {
						if(*this == mstruct[0]) {
							if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true))
							|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne())
							|| representsNonZero(true)
							|| mstruct[1].representsPositive()
							|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
									(*mparent)[index_this][1].number()++;
									(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
								} else {
									set_nocopy(mstruct, true);
									CHILD(1).number()++;
									calculateRaiseExponent(eo, mparent, index_this);
								}
								return 1;
							}
						} else {
							for(size_t i = 0; i < SIZE; i++) {
								int ret = CHILD(i).merge_multiplication(mstruct, eo, NULL, 0, 0, false, false);
								if(ret == 0) {
									ret = mstruct.merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
									if(ret >= 1) {
										if(ret == 2) ret = 3;
										else if(ret == 3) ret = 2;
										mstruct.ref();
										setChild_nocopy(&mstruct, i + 1);
									}
								}
								if(ret >= 1) {
									if(ret != 2) calculateMultiplyIndex(i, eo, true, mparent, index_this);
									return 1;
								}
							}
						}
					}
				}
				default: {
					if(do_append) {
						MERGE_APPROX_AND_PREC(mstruct)
						if(reversed) {
							PREPEND_REF(&mstruct);
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
						} else {
							APPEND_REF(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						}
						return 1;
					}
				}
			}
			return -1;
		}
		case STRUCT_POWER: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				case STRUCT_POWER: {
					if(CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ROOT && mstruct[1].isMinusOne() && CHILD(0).size() == 1 && VALID_ROOT(mstruct[0]) && CHILD(0)[0].equals(mstruct[0][0]) && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().isFraction() && CHILD(1).number().denominator() == mstruct[0][1].number() && CHILD(0)[0].representsReal(true) && CHILD(1).number().numerator() == -(mstruct[0][1].number() - 1)) {
						// root(x, 3)^-1*abs(x)^(-2/3)=1/x
						SET_CHILD_MAP(0)
						SET_CHILD_MAP(0)
						raise(m_minus_one);
						return 1;
					}
					if(mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ABS && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ROOT && CHILD(1).isMinusOne() && mstruct[0].size() == 1 && VALID_ROOT(CHILD(0)) && CHILD(0)[0].equals(mstruct[0][0]) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().isFraction() && mstruct[1].number().denominator() == CHILD(0)[1].number() && mstruct[0][0].representsReal(true) && mstruct[1].number().numerator() == -(CHILD(0)[1].number() - 1)) {
						// root(x, 3)^-1*abs(x)^(-2/3)=1/x
						SET_CHILD_MAP(0)
						SET_CHILD_MAP(0)
						raise(m_minus_one);
						return 1;
					}
					if(mstruct[0] == CHILD(0) || (CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1] == mstruct[0] && mstruct[1].representsEven())) {
						if(mstruct[0].isUnit() && mstruct[0].prefix()) CHILD(0).setPrefix(mstruct[0].prefix());
						bool b = eo.allow_complex || CHILD(0).representsNonNegative(true), b2 = true, b_warn = false;
						if(!b) {
							b = CHILD(1).representsInteger() && mstruct[1].representsInteger();
						}
						if(b) {
							b = false;
							bool b2test = false;
							if(IS_REAL(mstruct[1]) && IS_REAL(CHILD(1))) {
								if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
									b2 = true;
									b = true;
								} else if(!mstruct[1].number().isMinusOne() && !CHILD(1).number().isMinusOne()) {
									b2 = (mstruct[1].number() + CHILD(1).number()).isNegative();
									b = true;
									if(!b2) b2test = true;
								}
							}
							if(!b || b2test) {
								b = (!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))
								|| CHILD(0).representsNonZero(true)
								|| (CHILD(1).representsPositive() && mstruct[1].representsPositive())
								|| (CHILD(1).representsNegative() && mstruct[1].representsNegative());
								if(!b && eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) {
									b = true;
									b_warn = true;
								}
								if(b2test) {
									b2 = b;
									b = true;
								}
							}
						}
						if(b) {
							if(IS_REAL(CHILD(1)) && IS_REAL(mstruct[1])) {
								if(!b2 && !do_append) return -1;
								if(b_warn && !warn_about_denominators_assumed_nonzero(CHILD(0), eo)) return -1;
								if(b2) {
									CHILD(1).number() += mstruct[1].number();
									calculateRaiseExponent(eo, mparent, index_this);
								} else {
									if(CHILD(1).number().isNegative()) {
										CHILD(1).number()++;
										mstruct[1].number() += CHILD(1).number();
										CHILD(1).number().set(-1, 1, 0);
									} else {
										mstruct[1].number()++;
										CHILD(1).number() += mstruct[1].number();
										mstruct[1].number().set(-1, 1, 0);
									}
									MERGE_APPROX_AND_PREC(mstruct)
									transform(STRUCT_MULTIPLICATION);
									CHILD(0).calculateRaiseExponent(eo, this, 0);
									if(reversed) {
										PREPEND_REF(&mstruct);
										CHILD(0).calculateRaiseExponent(eo, this, 0);
										calculateMultiplyIndex(0, eo, true, mparent, index_this);
									} else {
										APPEND_REF(&mstruct);
										CHILD(1).calculateRaiseExponent(eo, this, 1);
										calculateMultiplyLast(eo, true, mparent, index_this);
									}
								}
								return 1;
							} else {
								MathStructure mstruct2(CHILD(1));
								if(mstruct2.calculateAdd(mstruct[1], eo)) {
									if(b_warn && !warn_about_denominators_assumed_nonzero_llgg(CHILD(0), CHILD(1), mstruct[1], eo)) return -1;
									CHILD(1) = mstruct2;
									calculateRaiseExponent(eo, mparent, index_this);
									return 1;
								}
							}
						}
					} else if(mstruct[1] == CHILD(1)) {
						if(!CHILD(0).isMultiplication() && !mstruct[0].isMultiplication() && (mstruct[1].representsInteger() || CHILD(0).representsPositive(true) || mstruct[0].representsPositive(true))) {
							MathStructure mstruct2(CHILD(0));
							if(mstruct2.calculateMultiply(mstruct[0], eo)) {
								CHILD(0) = mstruct2;
								MERGE_APPROX_AND_PREC(mstruct)
								calculateRaiseExponent(eo, mparent, index_this);
								return 1;
							}
						} else if(eo.transform_trigonometric_functions && CHILD(1).representsInteger() && CHILD(0).isFunction() && mstruct[0].isFunction() && eo.protected_function != mstruct[0].function() && eo.protected_function != CHILD(0).function() && CHILD(0).size() == 1 && mstruct[0].size() == 1 && CHILD(0)[0] == mstruct[0][0]) {
							if((CHILD(0).function()->id() == FUNCTION_ID_COS && mstruct[0].function()->id() == FUNCTION_ID_SIN) || (CHILD(0).function()->id() == FUNCTION_ID_SIN && mstruct[0].function()->id() == FUNCTION_ID_COS) || (CHILD(0).function()->id() == FUNCTION_ID_COSH && mstruct[0].function()->id() == FUNCTION_ID_SINH) || (CHILD(0).function()->id() == FUNCTION_ID_SINH && mstruct[0].function()->id() == FUNCTION_ID_COSH)) {
								// cos(x)^n*sin(x)^n=sin(2x)^n/2^n
								if(CHILD(0).function()->id() == FUNCTION_ID_COSH) CHILD(0).setFunctionId(FUNCTION_ID_SINH);
								else if(CHILD(0).function()->id() == FUNCTION_ID_COS) CHILD(0).setFunctionId(FUNCTION_ID_SIN);
								CHILD(0)[0].calculateMultiply(nr_two, eo);
								CHILD(0).childUpdated(1);
								CHILD_UPDATED(0)
								MathStructure *mdiv = new MathStructure(2, 1, 0);
								mdiv->calculateRaise(CHILD(1), eo);
								mdiv->calculateInverse(eo);
								multiply_nocopy(mdiv);
								calculateMultiplyLast(eo);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if((CHILD(0).function()->id() == FUNCTION_ID_TAN && mstruct[0].function()->id() == FUNCTION_ID_COS) || (CHILD(0).function()->id() == FUNCTION_ID_COS && mstruct[0].function()->id() == FUNCTION_ID_TAN)) {
								// tan(x)^n*cos(x)^n=sin(x)^n
								CHILD(0).setFunctionId(FUNCTION_ID_SIN);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if((CHILD(0).function()->id() == FUNCTION_ID_TANH && mstruct[0].function()->id() == FUNCTION_ID_COSH) || (CHILD(0).function()->id() == FUNCTION_ID_COSH && mstruct[0].function()->id() == FUNCTION_ID_TANH)) {
								// tanh(x)^n*cosh(x)^n=sinh(x)^n
								CHILD(0).setFunctionId(FUNCTION_ID_SIN);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					} else if(eo.transform_trigonometric_functions && CHILD(1).isInteger() && mstruct[1].isInteger() && CHILD(0).isFunction() && mstruct[0].isFunction() && eo.protected_function != mstruct[0].function() && eo.protected_function != CHILD(0).function() && mstruct[0].size() == 1 && CHILD(0).size() == 1 && CHILD(0)[0] == mstruct[0][0]) {
						if(CHILD(1).number().isNonNegative() != mstruct[1].number().isNonNegative()) {
							if(CHILD(0).function()->id() == FUNCTION_ID_SIN) {
								if(mstruct[0].function()->id() == FUNCTION_ID_COS) {
									CHILD(0).setFunctionId(FUNCTION_ID_TAN);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunctionId(FUNCTION_ID_SIN);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								} else if(mstruct[0].function()->id() == FUNCTION_ID_TAN) {
									CHILD(0).setFunctionId(FUNCTION_ID_COS);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunctionId(FUNCTION_ID_SIN);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function()->id() == FUNCTION_ID_COS) {
								if(mstruct[0].function()->id() == FUNCTION_ID_SIN) {
									mstruct[0].setFunctionId(FUNCTION_ID_TAN);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunctionId(FUNCTION_ID_SIN);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function()->id() == FUNCTION_ID_TAN) {
								if(mstruct[0].function()->id() == FUNCTION_ID_SIN) {
									mstruct[0].setFunctionId(FUNCTION_ID_COS);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunctionId(FUNCTION_ID_SIN);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function()->id() == FUNCTION_ID_SINH) {
								if(mstruct[0].function()->id() == FUNCTION_ID_COSH) {
									CHILD(0).setFunctionId(FUNCTION_ID_TANH);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunctionId(FUNCTION_ID_SINH);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								} else if(mstruct[0].function()->id() == FUNCTION_ID_TANH) {
									CHILD(0).setFunctionId(FUNCTION_ID_COSH);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunctionId(FUNCTION_ID_SINH);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function()->id() == FUNCTION_ID_COSH) {
								if(mstruct[0].function()->id() == FUNCTION_ID_SINH) {
									mstruct[0].setFunctionId(FUNCTION_ID_TANH);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunctionId(FUNCTION_ID_SINH);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function()->id() == FUNCTION_ID_TANH) {
								if(mstruct[0].function()->id() == FUNCTION_ID_SINH) {
									mstruct[0].setFunctionId(FUNCTION_ID_COSH);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunctionId(FUNCTION_ID_SINH);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							}
						} else {
							if((CHILD(0).function()->id() == FUNCTION_ID_TAN && mstruct[0].function()->id() == FUNCTION_ID_COS)) {
								// tan(x)^n*cos(x)^m=sin(x)^n*cos(x)^(m-n)
								CHILD(0).setFunctionId(FUNCTION_ID_SIN);
								mstruct[1].number() -= CHILD(1).number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function()->id() == FUNCTION_ID_COS && mstruct[0].function()->id() == FUNCTION_ID_TAN)) {
								// tan(x)^n*cos(x)^m=sin(x)^n*cos(x)^(m-n)
								mstruct[0].setFunctionId(FUNCTION_ID_SIN);
								CHILD(1).number() -= mstruct[1].number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function()->id() == FUNCTION_ID_TANH && mstruct[0].function()->id() == FUNCTION_ID_COSH)) {
								// tanh(x)^n*cosh(x)^m=sinh(x)^n*cosh(x)^(m-n)
								CHILD(0).setFunctionId(FUNCTION_ID_SINH);
								mstruct[1].number() -= CHILD(1).number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function()->id() == FUNCTION_ID_COSH && mstruct[0].function()->id() == FUNCTION_ID_TANH)) {
								// tanh(x)^n*cosh(x)^m=sinh(x)^n*cosh(x)^(m-n)
								mstruct[0].setFunctionId(FUNCTION_ID_SINH);
								CHILD(1).number() -= mstruct[1].number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					} else if(mstruct[0].isMultiplication() && mstruct[0].size() == 2 && mstruct[0][0].isMinusOne() && mstruct[0][1] == CHILD(0) && CHILD(1).representsEven()) {
						return 0;
					}
					break;
				}
				case STRUCT_FUNCTION: {
					if(!FUNCTION_PROTECTED(eo, FUNCTION_ID_SIGNUM) && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2 && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1 && mstruct[0] == CHILD(0)[0] && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().numeratorIsOne() && !CHILD(1).number().denominatorIsEven() && CHILD(0)[0].representsReal(true)) {
						setType(STRUCT_FUNCTION);
						setFunctionId(FUNCTION_ID_ROOT);
						CHILD(0).setToChild(1, true);
						CHILD(1).number().recip();
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					if(eo.transform_trigonometric_functions && CHILD(0).isFunction() && CHILD(0).size() == 1 && mstruct.size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isNegative() && eo.protected_function != mstruct.function() && eo.protected_function != CHILD(0).function()) {
						if(CHILD(0).function()->id() == FUNCTION_ID_SIN) {
							if(mstruct.function()->id() == FUNCTION_ID_COS && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_TAN);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_TAN);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if(mstruct.function()->id() == FUNCTION_ID_TAN && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_COS);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_COS);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_COS) {
							if(mstruct.function()->id() == FUNCTION_ID_SIN && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_TAN);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_TAN);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_TAN) {
							if(mstruct.function()->id() == FUNCTION_ID_SIN && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_COS);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_COS);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_SINH) {
							if(mstruct.function()->id() == FUNCTION_ID_COSH && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_TANH);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_TANH);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if(mstruct.function()->id() == FUNCTION_ID_TANH && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_COSH);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_COSH);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_COSH) {
							if(mstruct.function()->id() == FUNCTION_ID_SINH && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_TANH);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_TANH);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_TANH) {
							if(mstruct.function()->id() == FUNCTION_ID_SINH && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunctionId(FUNCTION_ID_COSH);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunctionId(FUNCTION_ID_COSH);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
					if(mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS && mstruct.size() == 1) {
						if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable()->isKnown() && mstruct[0].contains(CHILD(0), false) > 0) {
							if(separate_unit_vars(CHILD(0), eo, false)) {
								calculateRaiseExponent(eo);
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
				}
				default: {
					if(!mstruct.isNumber() && CHILD(1).isNumber() && CHILD(0) == mstruct) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))
						|| (CHILD(1).isNumber() && CHILD(1).number().isReal() && !CHILD(1).number().isMinusOne())
						|| CHILD(0).representsNonZero(true)
						|| CHILD(1).representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(CHILD(0), CHILD(1), eo))) {
							CHILD(1).number()++;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo, mparent, index_this);
							return 1;
						}
					}
					if(mstruct.isNumber() && CHILD(1).isNumber() && !CHILD(1).number().includesInfinity() && CHILD(0).isNumber() && CHILD(0).number().isRational() && !CHILD(0).number().isZero() && mstruct.number().isRational()) {
						if(CHILD(0).isInteger() && mstruct.number().denominator() == CHILD(0).number().numerator()) {
							CHILD(1).number()--;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo);
							if(!mstruct.number().numeratorIsOne()) calculateMultiply(mstruct.number().numerator(), eo, mparent, index_this);
							return 1;
						} else if(mstruct.number().denominator() == CHILD(0).number().numerator() && mstruct.number().numerator() == CHILD(0).number().denominator()) {
							CHILD(1).number()--;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo);
							return 1;
						}
					}
					if(mstruct.isZero() && (!eo.keep_zero_units || containsType(STRUCT_UNIT, false, true, true) <= 0 || (CHILD(0).isUnit() && CHILD(0).unit() == CALCULATOR->getRadUnit()) || (CHILD(0).isFunction() && CHILD(0).representsNumber(false))) && !representsUndefined(true, true, !eo.assume_denominators_nonzero) && representsNonMatrix()) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					if(CHILD(0).isFunction() && mstruct.isZero() && CHILD(1).representsNumber()) {
						if((CHILD(0).function()->id() == FUNCTION_ID_LOG || CHILD(0).function()->id() == FUNCTION_ID_EI) && SIZE == 1) {
							if(CHILD(0)[0].representsNonZero() || warn_about_assumed_not_value(CHILD(0)[0], m_zero, eo)) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 3;
							}
						} else if(CHILD(0).function()->id() == FUNCTION_ID_LI && SIZE == 1) {
							if(CHILD(0).representsNumber(true) || warn_about_assumed_not_value(CHILD(0)[0], m_one, eo)) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 3;
							}
						}
					}
					break;
				}
			}
			return -1;
		}
		case STRUCT_FUNCTION: {
			if(eo.protected_function != o_function) {
				if(((o_function->id() == FUNCTION_ID_ABS && SIZE == 1 && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_SIGNUM && CHILD(0).size() == 2) || (o_function->id() == FUNCTION_ID_SIGNUM && SIZE == 2 && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1)) && (CHILD(0)[0] == mstruct || (mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && CHILD(0)[0] == mstruct[0]) || (mstruct.isPower() && mstruct[0] == CHILD(0)[0]) || (mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ABS && mstruct[0].size() == 1 && CHILD(0)[0] == mstruct[0][0]))) {
					// sgn(abs(x))*x^y=x^y
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				} else if(o_function->id() == FUNCTION_ID_ABS && SIZE == 1) {
					if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						// abs(x)*abs(x)=x^2
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						calculateRaise(nr_two, eo);
						return 1;
					} else if(mstruct.isFunction() && eo.protected_function != mstruct.function() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2 && mstruct[0] == CHILD(0) && CHILD(0).representsScalar()) {
						// sgn(x)*abs(x)=x
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(o_function->id() == FUNCTION_ID_SIGNUM && SIZE == 2) {
					if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2 && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						if(mstruct[1].isOne() && CHILD(1).isOne()) {
							set(1, 1, 0, true);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct[1] == CHILD(1)) {
							// sgn(x)*sgn(x)=sgn(abs(x))
							CHILD(0).transformById(FUNCTION_ID_ABS);
							CHILD_UPDATED(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(mstruct.isFunction() && eo.protected_function != mstruct.function() && mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && mstruct[0] == CHILD(0) && CHILD(0).representsScalar()) {
						// sgn(x)*abs(x)=x
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct == CHILD(0) && CHILD(0).representsReal(true)) {
						// sgn(x)*x=abs(x)
						setFunctionId(FUNCTION_ID_ABS);
						ERASE(1)
						return 1;
					} else if(mstruct.isPower() && mstruct[1].representsOdd() && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						//sgn(x)*x^3=abs(x)^3
						mstruct[0].transformById(FUNCTION_ID_ABS);
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
						} else {
							set_nocopy(mstruct, true);
						}
						return 1;
					}
				} else if(o_function->id() == FUNCTION_ID_ROOT && THIS_VALID_ROOT) {
					if(CHILD(0) == mstruct && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)*x=abs(x)^(1/3)*x
						CHILD(0).transformById(FUNCTION_ID_ABS);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct.transformById(FUNCTION_ID_ABS);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isPower() && mstruct[1].representsOdd() && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)*x^3=abs(x)^(1/3)*x^3
						CHILD(0).transformById(FUNCTION_ID_ABS);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct[0].transformById(FUNCTION_ID_ABS);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct) && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && CHILD(1).number().isOdd() && mstruct[1].number().isOdd()) {
						// root(x, y)*root(x, z)=abs(x)^(1/y)*abs(x)^(1/z)
						CHILD(0).transformById(FUNCTION_ID_ABS);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct[0].transformById(FUNCTION_ID_ABS);
						mstruct[1].number().recip();
						mstruct.setType(STRUCT_POWER);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ABS && mstruct[0].size() == 1 && mstruct[0][0].equals(CHILD(0)) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().isFraction() && mstruct[0].number().denominator() == CHILD(1).number() && CHILD(0).representsReal(true) && mstruct[1].number().numerator() == CHILD(1).number() - 1) {
						// root(x, 3)*abs(x)^(2/3)=x
						SET_CHILD_MAP(0)
						return 1;
					}
				} else if((o_function->id() == FUNCTION_ID_LOG || o_function->id() == FUNCTION_ID_EI) && SIZE == 1 && mstruct.isZero()) {
					if(CHILD(0).representsNonZero() || warn_about_assumed_not_value(CHILD(0), m_zero, eo)) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
				} else if(o_function->id() == FUNCTION_ID_LI && SIZE == 1 && mstruct.isZero()) {
					if(representsNumber(true) || warn_about_assumed_not_value(CHILD(0), m_one, eo)) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
				} else if(eo.transform_trigonometric_functions && o_function->id() == FUNCTION_ID_SINC && SIZE == 1 && CHILD(0) == mstruct) {
					// sinc(x)*x=sin(x)
					CHILD(0).calculateMultiply(CALCULATOR->getRadUnit(), eo);
					CHILD_UPDATED(0)
					setFunctionId(FUNCTION_ID_SIN);
					if(eo.calculate_functions) calculateFunctions(eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(eo.transform_trigonometric_functions && mstruct.isFunction() && mstruct.size() == 1 && eo.protected_function != mstruct.function()) {
					if(o_function->id() == FUNCTION_ID_TAN && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_COS && mstruct[0] == CHILD(0)) {
							// tan(x)*cos(x)=sin(x)
							setFunctionId(FUNCTION_ID_SIN);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function->id() == FUNCTION_ID_COS && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_TAN && mstruct[0] == CHILD(0)) {
							// tan(x)*cos(x)=sin(x)
							setFunctionId(FUNCTION_ID_SIN);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.function()->id() == FUNCTION_ID_SIN && mstruct[0] == CHILD(0)) {
							// cos(x)*sin(x)=sin(2x)/2
							setFunctionId(FUNCTION_ID_SIN);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function->id() == FUNCTION_ID_SIN && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_COS && mstruct[0] == CHILD(0)) {
							// cos(x)*sin(x)=sin(2x)/2
							setFunctionId(FUNCTION_ID_SIN);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function->id() == FUNCTION_ID_TANH && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_COSH && mstruct[0] == CHILD(0)) {
							// tanh(x)*cosh(x)=sinh(x)
							setFunctionId(FUNCTION_ID_SINH);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function->id() == FUNCTION_ID_SINH && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_COSH && mstruct[0] == CHILD(0)) {
							// cosh(x)*sinh(x)=sinh(2x)/2
							setFunctionId(FUNCTION_ID_SINH);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function->id() == FUNCTION_ID_COSH && SIZE == 1) {
						if(mstruct.function()->id() == FUNCTION_ID_TANH && mstruct[0] == CHILD(0)) {
							// tanh(x)*cosh(x)=sinh(x)
							setFunctionId(FUNCTION_ID_SINH);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.function()->id() == FUNCTION_ID_SINH && mstruct[0] == CHILD(0)) {
							// cosh(x)*sinh(x)=sinh(2x)/2
							setFunctionId(FUNCTION_ID_SINH);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				}
			}
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) {
				if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS && mstruct.size() == 1) {
					mstruct[0].ref();
					CHILD(0).multiply_nocopy(&mstruct[0]);
					return 1;
				} else if(mstruct.isVariable() && mstruct.variable()->isKnown() && CHILD(0).contains(mstruct, false) > 0) {
					if(separate_unit_vars(mstruct, eo, false)) {
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					}
				} else if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable()->isKnown() && CHILD(0).contains(mstruct[0], false) > 0) {
					if(separate_unit_vars(mstruct[0], eo, false)) {
						mstruct.calculateRaiseExponent(eo);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					}
				}
			}
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {}
				case STRUCT_POWER: {
					return 0;
				}
				case STRUCT_COMPARISON: {
					if(isComparison()) {
						mstruct.ref();
						transform_nocopy(STRUCT_LOGICAL_AND, &mstruct);
						return 1;
					}
				}
				default: {
					if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS && mstruct.size() == 1) {
						if(m_type == STRUCT_VARIABLE && o_variable->isKnown() && mstruct[0].contains(*this, false) > 0) {
							if(separate_unit_vars(*this, eo, false)) {
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
					if(mstruct.isZero() && (!eo.keep_zero_units || containsType(STRUCT_UNIT, false, true, true) <= 0 || (isUnit() && unit() == CALCULATOR->getRadUnit()) || (isFunction() && representsNumber(false))) && !representsUndefined(true, true, !eo.assume_denominators_nonzero) && representsNonMatrix()) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
					if(isZero() && !mstruct.representsUndefined(true, true, !eo.assume_denominators_nonzero) && (!eo.keep_zero_units || mstruct.containsType(STRUCT_UNIT, false, true, true) <= 0 || (mstruct.isUnit() && mstruct.unit() == CALCULATOR->getRadUnit())) && mstruct.representsNonMatrix()) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					}
					if(equals(mstruct)) {
						if(mstruct.isUnit() && mstruct.prefix()) o_prefix = mstruct.prefix();
						raise_nocopy(new MathStructure(2, 1, 0));
						MERGE_APPROX_AND_PREC(mstruct)
						calculateRaiseExponent(eo, mparent, index_this);
						return 1;
					}
					break;
				}
			}
			break;
		}
	}
	return -1;
}

bool test_if_numerator_not_too_large(Number &vb, Number &ve) {
	if(!vb.isRational()) return false;
	if(!mpz_fits_slong_p(mpq_numref(ve.internalRational()))) return false;
	long int exp = labs(mpz_get_si(mpq_numref(ve.internalRational())));
	if(vb.isRational()) {
		if((long long int) exp * mpz_sizeinbase(mpq_numref(vb.internalRational()), 10) <= 1000000LL && (long long int) exp * mpz_sizeinbase(mpq_denref(vb.internalRational()), 10) <= 1000000LL) return true;
	}
	return false;
}

bool is_negation(const MathStructure &m1, const MathStructure &m2) {
	if(m1.isAddition() && m2.isAddition() && m1.size() == m2.size()) {
		for(size_t i = 0; i < m1.size(); i++) {
			if(!is_negation(m1[i], m2[i])) return false;
		}
		return true;
	}
	if(m1.isNumber() && m2.isNumber()) {
		return m1.number() == -m2.number();
	}
	if(m1.isMultiplication() && m1.size() > 1) {
		if(m1[0].isNumber()) {
			if(m1[0].number().isMinusOne()) {
				if(m1.size() == 2) return m1[1] == m2;
				if(m2.isMultiplication() && m2.size() == m1.size() - 1) {
					for(size_t i = 1; i < m1.size(); i++) {
						if(!m1[i].equals(m2[i - 1], true, true)) return false;
					}
					return true;
				}
				return false;
			} else {
				if(m2.isMultiplication() && m2.size() == m1.size() && m2[0].isNumber()) {
					for(size_t i = 1; i < m1.size(); i++) {
						if(!m1[i].equals(m2[i], true, true)) return false;
					}
					return m1[0].number().equals(-m2[0].number(), true, true);
				}
				return false;
			}
		}
	}
	if(m2.isMultiplication() && m2.size() > 1) {
		if(m2[0].isNumber()) {
			if(m2[0].number().isMinusOne()) {
				if(m2.size() == 2) return m2[1] == m1;
				if(m1.isMultiplication() && m1.size() == m2.size() - 1) {
					for(size_t i = 1; i < m2.size(); i++) {
						if(!m2[i].equals(m1[i - 1], true, true)) return false;
					}
					return true;
				}
				return false;
			}
		}
	}
	return false;
}

int MathStructure::merge_power(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		// number^number
		Number nr(o_number);
		if(nr.raise(mstruct.number(), eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			// Exponentiation succeeded without inappropriate change in approximation status
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		if(!o_number.isMinusOne() && !o_number.isOne() && mstruct.number().isRational() && !mstruct.isInteger()) {
			if(o_number.isNegative()) {
				MathStructure mtest(*this);
				if(mtest.number().negate() && mtest.calculateRaise(mstruct, eo)) {
					set(mtest);
					MathStructure *mmul = new MathStructure(-1, 1, 0);
					mmul->calculateRaise(mstruct, eo);
					multiply_nocopy(mmul);
					calculateMultiplyLast(eo);
					return 1;
				}
			} else {
				Number exp_num(mstruct.number().numerator());
				if(!exp_num.isOne() && !exp_num.isMinusOne() && o_number.isPositive() && test_if_numerator_not_too_large(o_number, exp_num)) {
					// Try raise by exponent numerator if not very large
					nr = o_number;
					if(nr.raise(exp_num, eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
						o_number = nr;
						numberUpdated();
						nr.set(mstruct.number().denominator());
						nr.recip();
						calculateRaise(nr, eo, mparent, index_this);
						return 1;
					}
				}
				if(o_number.isPositive()) {
					Number nr_root(mstruct.number().denominator());
					if(eo.split_squares && o_number.isInteger() && o_number.integerLength() < 100000L && nr_root.isLessThanOrEqualTo(LARGEST_RAISED_PRIME_EXPONENT)) {
						int root = nr_root.intValue();
						nr.set(1, 1, 0);
						bool b = true, overflow;
						long int val;
						while(b) {
							if(CALCULATOR->aborted()) break;
							b = false;
							overflow = false;
							val = o_number.lintValue(&overflow);
							if(overflow) {
								mpz_srcptr cval = mpq_numref(o_number.internalRational());
								for(size_t i = 0; root == 2 ? (i < NR_OF_SQUARE_PRIMES) : (RAISED_PRIMES[root - 3][i] != 0); i++) {
									if(CALCULATOR->aborted()) break;
									if(mpz_divisible_ui_p(cval, (unsigned long int) (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]))) {
										nr *= PRIMES[i];
										o_number /= (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]);
										b = true;
										break;
									}
								}
							} else {
								for(size_t i = 0; root == 2 ? (i < NR_OF_SQUARE_PRIMES) : (RAISED_PRIMES[root - 3][i] != 0); i++) {
									if(CALCULATOR->aborted()) break;
									if((root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]) > val) {
										break;
									} else if(val % (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]) == 0) {
										nr *= PRIMES[i];
										o_number /= (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]);
										b = true;
										break;
									}
								}
							}
						}
						if(!nr.isOne()) {
							transform(STRUCT_MULTIPLICATION);
							CHILD(0).calculateRaise(mstruct, eo, this, 0);
							PREPEND(nr);
							if(!mstruct.number().numeratorIsOne()) {
								CHILD(0).calculateRaise(mstruct.number().numerator(), eo, this, 0);
							}
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
							return 1;
						}
					}
					if(eo.split_squares && nr_root != 2) {
						// partial roots, e.g. 9^(1/4)=3^(1/2)
						if(nr_root.isEven()) {
							Number nr(o_number);
							if(nr.sqrt() && !nr.isApproximate()) {
								o_number = nr;
								mstruct.number().multiply(2);
								mstruct.ref();
								raise_nocopy(&mstruct);
								calculateRaiseExponent(eo, mparent, index_this);
								return 1;
							}
						}
						for(size_t i = 1; i < NR_OF_PRIMES; i++) {
							if(nr_root.isLessThanOrEqualTo(PRIMES[i])) break;
							if(nr_root.isIntegerDivisible(PRIMES[i])) {
								Number nr(o_number);
								if(nr.root(Number(PRIMES[i], 1)) && !nr.isApproximate()) {
									o_number = nr;
									mstruct.number().multiply(PRIMES[i]);
									mstruct.ref();
									raise_nocopy(&mstruct);
									calculateRaiseExponent(eo, mparent, index_this);
									return 1;
								}
							}
						}
					}
				}
			}
		}
		if(o_number.isMinusOne() && mstruct.number().isRational()) {
			if(mstruct.number().isInteger()) {
				if(mstruct.number().isEven()) set(m_one, true);
				else set(m_minus_one, true);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else {
				Number nr_floor(mstruct.number());
				nr_floor.floor();
				if(mstruct.number().denominatorIsTwo()) {
					if(nr_floor.isEven()) set(nr_one_i, true);
					else set(nr_minus_i, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else {
					mstruct.number() -= nr_floor;
					mstruct.numberUpdated();
					if(mstruct.number().denominator() == 3) {
						set(3, 1, 0, true);
						calculateRaise(nr_half, eo);
						if(nr_floor.isEven()) calculateMultiply(nr_one_i, eo);
						else calculateMultiply(nr_minus_i, eo);
						calculateMultiply(nr_half, eo);
						if(nr_floor.isEven() == mstruct.number().numeratorIsOne()) calculateAdd(nr_half, eo);
						else calculateAdd(nr_minus_half, eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct.number().denominator() == 4) {
						if(nr_floor.isEven() == mstruct.number().numeratorIsOne()) set(1, 1, 0, true);
						else set(-1, 1, 0, true);
						if(nr_floor.isEven()) calculateAdd(nr_one_i, eo);
						else calculateAdd(nr_minus_i, eo);
						multiply(nr_two);
						LAST.calculateRaise(nr_minus_half, eo);
						calculateMultiplyLast(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(!nr_floor.isZero()) {
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						if(nr_floor.isOdd()) calculateNegate(eo);
						return 1;
					}
				}
			}
		}
		if(o_number.isRational() && !o_number.isInteger() && !o_number.numeratorIsOne() && mstruct.number().isRational()) {
			Number num(o_number.numerator());
			Number den(o_number.denominator());
			if(den.integerLength() < 100000L && num.integerLength() < 100000L) {
				set(num, true);
				calculateRaise(mstruct, eo);
				multiply(den);
				LAST.calculateRaise(mstruct, eo);
				LAST.calculateInverse(eo);
				calculateMultiplyLast(eo);
				return 1;
			}
		}
		// If base numerator is larger than denominator, invert base and negate exponent
		if(o_number.isRational() && !o_number.isInteger() && !o_number.isZero() && ((o_number.isNegative() && o_number.isGreaterThan(nr_minus_one) && mstruct.number().isInteger()) || (o_number.isPositive() && o_number.isLessThan(nr_one)))) {
			mstruct.number().negate();
			o_number.recip();
			return 0;
		}
		return -1;
	}

	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	} else if(isOne() && mstruct.representsNumber()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite(false)) {
		if(mstruct.representsNegative(false)) {
			o_number.clear();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		} else if(mstruct.representsNonZero(false) && mstruct.representsPositive(false)) {
			if(o_number.isMinusInfinity()) {
				if(mstruct.representsEven(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsOdd(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity()) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		MathStructure mtest(mstruct);
		CALCULATOR->beginTemporaryEnableIntervalArithmetic();
		if(CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->beginTemporaryStopMessages();
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			mtest.calculateFunctions(eo2);
			mtest.calculatesub(eo2, eo2);
			CALCULATOR->endTemporaryStopMessages();
		}
		CALCULATOR->endTemporaryEnableIntervalArithmetic();
		if(mtest.representsNegative(false)) {
			o_number.clear();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		} else if(mtest.representsNonZero(false) && mtest.representsPositive(false)) {
			if(o_number.isMinusInfinity()) {
				if(mstruct.representsEven(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsOdd(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity()) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite(false)) {
		MathStructure mtest(*this);
		CALCULATOR->beginTemporaryEnableIntervalArithmetic();
		if(CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->beginTemporaryStopMessages();
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			mtest.calculateFunctions(eo2);
			mtest.calculatesub(eo2, eo2);
			CALCULATOR->endTemporaryStopMessages();
		}
		CALCULATOR->endTemporaryEnableIntervalArithmetic();
		if(mtest.isNumber()) {
			if(mtest.merge_power(mstruct, eo) > 0 && mtest.isNumber()) {
				if(mtest.number().isPlusInfinity()) {
					set(nr_plus_inf, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.number().isMinusInfinity()) {
					set(nr_minus_inf, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.number().isZero()) {
					clear(true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	}

	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	if(isZero() && mstruct.representsPositive()) {
		return 1;
	}
	if(mstruct.isZero() && !representsUndefined(true, true)) {
		set(m_one);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}

	switch(m_type) {
		case STRUCT_VECTOR: {
			if(mstruct.isNumber() && mstruct.number().isInteger()) {
				if(isMatrix()) {
					if(matrixIsSquare()) {
						Number nr(mstruct.number());
						bool b_neg = false;
						if(nr.isNegative()) {
							nr.setNegative(false);
							b_neg = true;
						}
						if(!nr.isOne()) {
							MathStructure msave(*this);
							nr--;
							while(nr.isPositive()) {
								if(CALCULATOR->aborted()) {
									set(msave);
									return -1;
								}
								calculateMultiply(msave, eo);
								nr--;
							}
						}
						if(b_neg) {
							if(!invertMatrix(eo)) {
								if(mstruct.number().isMinusOne()) return -1;
								raise(nr_minus_one);
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					return -1;
				} else {
					if(mstruct.number().isMinusOne()) {
						return -1;
					}
					Number nr(mstruct.number());
					if(nr.isNegative()) {
						nr++;
					} else {
						nr--;
					}
					MathStructure msave(*this);
					calculateMultiply(msave, eo);
					calculateRaise(nr, eo);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}

			}
			goto default_power_merge;
		}
		case STRUCT_ADDITION: {
			if(mstruct.isNumber() && mstruct.number().isInteger() && containsType(STRUCT_DATETIME, false, true, false) <= 0) {
				if(eo.reduce_divisions && mstruct.number().isMinusOne()) {
					int bnum = -1, bden = -1;
					int inegs = 0;
					bool b_break = false;
					for(size_t i = 0; i < SIZE && !b_break; i++) {
						switch(CHILD(i).type()) {
							case STRUCT_NUMBER: {
								if(!CHILD(i).number().isRational() || CHILD(i).number().numeratorIsGreaterThan(1000000L) || CHILD(i).number().numeratorIsLessThan(-1000000L) || CHILD(i).number().denominatorIsGreaterThan(1000L)) {
									bnum = 0; bden = 0; inegs = 0; b_break = true;
								}
								if(bden != 0 && !CHILD(i).number().isInteger()) bden = 1;
								if(bnum != 0 && !CHILD(i).isZero()) {
									if(CHILD(i).number().numeratorIsOne() || CHILD(i).number().numeratorIsMinusOne()) bnum = 0;
									else bnum = 1;
								}
								if(CHILD(i).number().isNegative()) {
									inegs++;
									if(i == 0) inegs++;
								} else if(!CHILD(i).number().isZero()) inegs--;
								break;
							}
							case STRUCT_MULTIPLICATION: {
								if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
									if(!CHILD(i)[0].number().isRational() || CHILD(i)[0].number().numeratorIsGreaterThan(1000000L) || CHILD(i)[0].number().numeratorIsLessThan(-1000000L) || CHILD(i)[0].number().denominatorIsGreaterThan(1000L)) {
										bnum = 0; bden = 0; inegs = 0; b_break = true;
									}
									if(bden != 0 && !CHILD(i)[0].number().isInteger()) bden = 1;
									if(bnum != 0 && !CHILD(i)[0].isZero()) {
										if(CHILD(i)[0].number().numeratorIsOne() || CHILD(i)[0].number().numeratorIsMinusOne()) bnum = 0;
										else bnum = 1;
									}
									if(CHILD(i)[0].number().isNegative()) {
										inegs++;
										if(i == 0) inegs++;
									} else if(!CHILD(i)[0].number().isZero()) inegs--;
									break;
								}
							}
							default: {
								bnum = 0;
								inegs--;
								break;
							}
						}
					}
					if(bden < 0) bden = 0;
					if(bnum < 0) bnum = 0;
					if(bnum || bden) {
						Number nr_num, nr_den(1, 1, 0);
						for(size_t i = 0; i < SIZE && !nr_den.isZero(); i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {
									if(CHILD(i).number().isInteger()) {
										if(bnum && !nr_num.isOne() && !CHILD(i).number().isZero()) {
											if(nr_num.isZero()) nr_num = CHILD(i).number();
											else nr_num.gcd(CHILD(i).number());
										}
									} else {
										if(bnum && !nr_num.isOne() && !CHILD(i).number().isZero()) {
											if(nr_num.isZero()) nr_num = CHILD(i).number().numerator();
											else nr_num.gcd(CHILD(i).number().numerator());
										}
										if(bden) {
											nr_den.lcm(CHILD(i).number().denominator());
											if(nr_den.isGreaterThan(1000000L)) nr_den.clear();
										}
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
										if(CHILD(i)[0].number().isInteger()) {
											if(bnum && !nr_num.isOne() && !CHILD(i)[0].number().isZero()) {
												if(nr_num.isZero()) nr_num = CHILD(i)[0].number();
												else nr_num.gcd(CHILD(i)[0].number());
											}
										} else {
											if(bnum && !nr_num.isOne() && !CHILD(i)[0].number().isZero()) {
												if(nr_num.isZero()) nr_num = CHILD(i)[0].number().numerator();
												else nr_num.gcd(CHILD(i)[0].number().numerator());
											}
											if(bden) {
												nr_den.lcm(CHILD(i)[0].number().denominator());
												if(nr_den.isGreaterThan(1000000L)) nr_den.clear();
											}
										}
										break;
									}
								}
								default: {
									break;
								}
							}
						}
						if(!nr_den.isZero() && (!nr_den.isOne() || !nr_num.isOne())) {
							Number nr(nr_den);
							nr.divide(nr_num);
							nr.setNegative(inegs > 0);
							for(size_t i = 0; i < SIZE; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_NUMBER: {
										CHILD(i).number() *= nr;
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
											CHILD(i)[0].number() *= nr;
											CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
											break;
										}
									}
									default: {
										CHILD(i).calculateMultiply(nr, eo);
									}
								}
							}
							calculatesub(eo, eo, false);
							mstruct.ref();
							raise_nocopy(&mstruct);
							calculateRaiseExponent(eo);
							calculateMultiply(nr, eo, mparent, index_this);
							return 1;
						}
					}
					if(inegs > 0) {
						for(size_t i = 0; i < SIZE; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {CHILD(i).number().negate(); break;}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
										CHILD(i)[0].number().negate();
										CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
										break;
									}
								}
								default: {
									CHILD(i).calculateNegate(eo);
								}
							}
						}
						mstruct.ref();
						raise_nocopy(&mstruct);
						negate();
						return 1;
					}
				} else if(eo.expand != 0 && !mstruct.number().isZero() && (eo.expand > -2 || !containsInterval())) {
					bool b = true;
					bool neg = mstruct.number().isNegative();
					Number m(mstruct.number());
					m.setNegative(false);
					if(SIZE > 1) {
						if(eo.expand == -1) {
							switch(SIZE) {
								case 4: {if(m.isGreaterThan(3)) {b = false;} break;}
								case 3: {if(m.isGreaterThan(4)) {b = false;} break;}
								case 2: {if(m.isGreaterThan(10)) {b = false;} break;}
								default: {
									if(SIZE > 8 || m.isGreaterThan(2)) b = false;
								}
							}
						} else {
							b = false;
							long int i_pow = m.lintValue(&b);
							if(b || i_pow > 300) {
								b = false;
							} else {
								Number num_terms;
								if(num_terms.binomial(i_pow + (long int) SIZE - 1, (long int) SIZE - 1)) {
									size_t tc = countTotalChildren() / SIZE;
									if(tc <= 4) tc = 0;
									else tc -= 4;
									b = num_terms.isLessThanOrEqualTo(tc > 1 ? 300 / tc : 300);
								}
							}
						}
					}
					if(b) {
						if(!representsNonMatrix()) {
							MathStructure mthis(*this);
							while(!m.isOne()) {
								if(CALCULATOR->aborted()) {
									set(mthis);
									goto default_power_merge;
								}
								calculateMultiply(mthis, eo);
								m--;
							}
						} else {
							MathStructure mstruct1(CHILD(0));
							MathStructure mstruct2(CHILD(1));
							for(size_t i = 2; i < SIZE; i++) {
								if(CALCULATOR->aborted()) goto default_power_merge;
								mstruct2.add(CHILD(i), true);
							}
							Number k(1);
							Number p1(m);
							Number p2(1);
							p1--;
							Number bn;
							MathStructure msave(*this);
							CLEAR
							APPEND(mstruct1);
							CHILD(0).calculateRaise(m, eo);
							while(k.isLessThan(m)) {
								if(CALCULATOR->aborted() || !bn.binomial(m, k)) {
									set(msave);
									goto default_power_merge;
								}
								APPEND_NEW(bn);
								LAST.multiply(mstruct1);
								if(!p1.isOne()) {
									LAST[1].raise_nocopy(new MathStructure(p1));
									LAST[1].calculateRaiseExponent(eo);
								}
								LAST.multiply(mstruct2, true);
								if(!p2.isOne()) {
									LAST[2].raise_nocopy(new MathStructure(p2));
									LAST[2].calculateRaiseExponent(eo);
								}
								LAST.calculatesub(eo, eo, false);
								k++;
								p1--;
								p2++;
							}
							APPEND(mstruct2);
							LAST.calculateRaise(m, eo);
							calculatesub(eo, eo, false);
						}
						if(neg) calculateInverse(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				}
			}
			goto default_power_merge;
		}
		case STRUCT_MULTIPLICATION: {
			if(mstruct.representsInteger()) {
				// (xy)^a=x^a*y^a
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculateRaise(mstruct, eo);
				}
				MERGE_APPROX_AND_PREC(mstruct)
				calculatesub(eo, eo, false, mparent, index_this);
				return 1;
			} else if(!mstruct.isInfinite()) {
				// (-5xy)^z=5^z*x^z*(-y)^z && x >= 0 && y<0
				MathStructure mnew;
				mnew.setType(STRUCT_MULTIPLICATION);
				for(size_t i = 0; i < SIZE;) {
					if(CHILD(i).representsNonNegative(true)) {
						CHILD(i).ref();
						mnew.addChild_nocopy(&CHILD(i));
						ERASE(i);
					} else if(CHILD(i).isNumber() && CHILD(i).number().isNegative() && !CHILD(i).number().isMinusOne()) {
						// (-5)^z=5^z*(-1)^z
						CHILD(i).number().negate();
						mnew.addChild(CHILD(i));
						CHILD(i).number().set(-1, 1, 0);
						i++;
					} else {
						i++;
					}
				}
				if(mnew.size() > 0) {
					if(SIZE > 0) {
						if(SIZE == 1) SET_CHILD_MAP(0)
						mnew.addChild(*this);
					}
					set_nocopy(mnew, true);
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateRaise(mstruct, eo);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
			goto default_power_merge;
		}
		case STRUCT_POWER: {
			if((eo.allow_complex && CHILD(1).representsFraction()) || (mstruct.representsInteger() && (eo.allow_complex || CHILD(0).representsInteger())) || representsNonNegative(true)) {
				if((((!eo.assume_denominators_nonzero || eo.warn_about_denominators_assumed_nonzero) && !CHILD(0).representsNonZero(true)) || CHILD(0).isZero()) && CHILD(1).representsNegative(true)) {
					if(!eo.assume_denominators_nonzero || CHILD(0).isZero() || !warn_about_denominators_assumed_nonzero(CHILD(0), eo)) break;
				}
				if(!CHILD(1).representsNonInteger() && !mstruct.representsInteger()) {
					if(CHILD(1).representsEven() && CHILD(0).representsReal(true)) {
						if(CHILD(0).representsNegative(true)) {
							CHILD(0).calculateNegate(eo);
						} else if(!CHILD(0).representsNonNegative(true)) {
							MathStructure mstruct_base(CHILD(0));
							CHILD(0).set(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), &mstruct_base, NULL);
						}
					} else if(!CHILD(1).representsOdd() && !CHILD(0).representsNonNegative(true)) {
						goto default_power_merge;
					}
				}
				mstruct.ref();
				MERGE_APPROX_AND_PREC(mstruct)
				CHILD(1).multiply_nocopy(&mstruct, true);
				CHILD(1).calculateMultiplyLast(eo, true, this, 1);
				calculateRaiseExponent(eo, mparent, index_this);
				return 1;
			}
			if(mstruct.isNumber() && CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart() && mstruct.number().isReal()) {
				CALCULATOR->beginTemporaryEnableIntervalArithmetic();
				if(CALCULATOR->usesIntervalArithmetic()) {
					CALCULATOR->beginTemporaryStopMessages();
					Number nr(*CHILD(1).number().internalImaginary());
					Number nrpi; nrpi.pi();
					nr.add(nrpi);
					nr.divide(nrpi);
					nr.divide(2);
					Number nr_u(nr.upperEndPoint());
					nr = nr.lowerEndPoint();
					nr_u.floor();
					nr.floor();
					if(!CALCULATOR->endTemporaryStopMessages() && nr == nr_u) {
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						nr.setApproximate(false);
						nr *= 2;
						nr.negate();
						nr *= nr_one_i;
						if(!nr.isZero()) {
							CHILD(1) += nr;
							CHILD(1).last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
						}
						mstruct.ref();
						CHILD(1).multiply_nocopy(&mstruct, true);
						CHILD(1).calculateMultiplyLast(eo, true, this, 1);
						calculateRaiseExponent(eo, mparent, index_this);
						return true;
					}
				}
				CALCULATOR->endTemporaryEnableIntervalArithmetic();
			}
			goto default_power_merge;
		}
		case STRUCT_VARIABLE: {
			if(o_variable->id() == VARIABLE_ID_E) {
				if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber()) {
					if(mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI) {
						if(mstruct[0].number().isI() || mstruct[0].number().isMinusI()) {
							//e^(i*pi)=-1
							set(m_minus_one, true);
							return 1;
						} else if(mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().hasRealPart() && mstruct[0].number().internalImaginary()->isRational()) {
							// e^(a*i*pi)=(-1)^(a)
							set(-1, 1, 0, true);
							calculateRaise(*mstruct[0].number().internalImaginary(), eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(mstruct[0].number().isI() && mstruct[1].isFunction() && mstruct[1].function()->id() == FUNCTION_ID_ATAN && mstruct[1].size() == 1 && !mstruct[1][0].containsUnknowns() && ((eo.expand != 0 && eo.expand > -2) || !mstruct[1][0].containsInterval(true, false, false, eo.expand == -2 ? 1 : 0))) {
						set(mstruct[1][0], true);
						calculateRaise(nr_two, eo);
						calculateAdd(m_one, eo);
						calculateRaise(nr_half, eo);
						calculateInverse(eo);
						multiply(mstruct[1][0]);
						LAST.calculateMultiply(nr_one_i, eo);
						LAST.calculateAdd(m_one, eo);
						calculateMultiplyLast(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return true;
					}
				} else if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_LOG && mstruct.size() == 1) {
					if(mstruct[0].representsNumber() && (eo.allow_infinite || mstruct[0].representsNonZero())) {
						// e^ln(x)=x; x!=0
						set_nocopy(mstruct[0], true);
						return 1;
					}
				}
			}
			goto default_power_merge;
		}
		case STRUCT_FUNCTION: {
			if(eo.protected_function != o_function) {
				if(o_function->id() == FUNCTION_ID_ABS && SIZE == 1) {
					if(mstruct.representsEven() && CHILD(0).representsReal(true)) {
						// abs(x)^2=x^2
						SET_CHILD_MAP(0);
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						return 1;
					}
				} else if(o_function->id() == FUNCTION_ID_SIGNUM && CHILD(0).representsReal(true) && SIZE == 2 && ((CHILD(1).isZero() && mstruct.representsPositive()) || CHILD(1).isOne())) {
					if(mstruct.representsOdd()) {
						// sgn(x)^3=sgn(x)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct.representsEven()) {
						if(CHILD(1).isOne() && CHILD(0).representsReal(true)) {
							SET_CHILD_MAP(0)
							return 1;
						} else {
							// sgn(x)^2=sgn(abs(x))
							CHILD(0).transformById(FUNCTION_ID_ABS);
							CHILD_UPDATED(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				} else if(o_function->id() == FUNCTION_ID_ROOT && THIS_VALID_ROOT) {
					if(mstruct.representsEven() && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)^2=abs(x)^(3/2)
						CHILD(0).transformById(FUNCTION_ID_ABS);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						return 1;
					} else if(mstruct.isNumber() && mstruct.number().isInteger() && !mstruct.number().isMinusOne()) {
						if(mstruct == CHILD(1)) {
							// root(x, a)^a=x
							SET_CHILD_MAP(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.number().isIntegerDivisible(CHILD(1).number())) {
							// root(x, a)^(2a)=x^2
							mstruct.calculateDivide(CHILD(1).number(), eo);
							mstruct.ref();
							SET_CHILD_MAP(0)
							raise_nocopy(&mstruct);
							return 1;
						} else if(CHILD(1).number().isIntegerDivisible(mstruct.number())) {
							// root(x, 3a)^(a)=root(x, 3)
							Number nr(CHILD(1).number());
							if(nr.divide(mstruct.number())) {
								CHILD(1) = nr;
								CHILD_UPDATED(1)
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					}
				}
			}
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1 && mstruct.containsType(STRUCT_UNIT, false, true, true) == 0) {
				mstruct.ref();
				CHILD(0).raise_nocopy(&mstruct);
				return 1;
			}
			goto default_power_merge;
		}
		default: {
			default_power_merge:

			if(mstruct.isAddition()) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					bool bneg = representsNegative(true);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger() && (!bneg || !eo.allow_complex || !mstruct[i].isNumber() || !mstruct[i].number().isRational() || !mstruct[i].number().denominatorIsEven())) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure msave(*this);
					clear(true);
					m_type = STRUCT_MULTIPLICATION;
					MERGE_APPROX_AND_PREC(mstruct)
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND(msave);
						mstruct[i].ref();
						LAST.raise_nocopy(&mstruct[i]);
						LAST.calculateRaiseExponent(eo);
						calculateMultiplyLast(eo, false);
					}
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
			} else if(mstruct.isMultiplication() && mstruct.size() > 1) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger()) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure mthis(*this);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(i == 0) mthis.raise(mstruct[i]);
						if(!mstruct[i].representsReal(true) || (isZero() && !mstruct[i].representsPositive(true))) continue;
						if(i > 0) mthis[1] = mstruct[i];
						EvaluationOptions eo2 = eo;
						eo2.split_squares = false;
						// avoid abs(x)^(2y) loop
						if(mthis.calculateRaiseExponent(eo2) && (!mthis.isPower() || ((!isFunction() || o_function->id() != FUNCTION_ID_ABS || SIZE != 1 || !CHILD(0).equals(mthis[0], true, true)) && (!is_negation(mthis[0], *this))))) {
							set(mthis);
							if(mstruct.size() == 2) {
								if(i == 0) {
									mstruct[1].ref();
									raise_nocopy(&mstruct[1]);
								} else {
									mstruct[0].ref();
									raise_nocopy(&mstruct[0]);
								}
							} else {
								mstruct.ref();
								raise_nocopy(&mstruct);
								CHILD(1).delChild(i + 1);
							}
							calculateRaiseExponent(eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				}
			} else if(mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().isInteger() && !mstruct.number().numeratorIsOne() && !mstruct.number().numeratorIsMinusOne()) {
				if(representsNonNegative(true) && (m_type != STRUCT_FUNCTION || o_function->id() != FUNCTION_ID_ABS)) {
					if(isMultiplication() && SIZE == 2 && CHILD(0).isMinusOne() && mstruct.number().numeratorIsEven()) {
						bool b;
						if(mstruct.number().isNegative()) {
							MathStructure mtest(CHILD(1));
							b = mtest.calculateRaise(-mstruct.number().numerator(), eo);
							if(b && mtest.isPower() && mtest[1] == -mstruct.number().numerator()) b = false;
							if(!b) break;
							set(mtest, true);
							raise(m_minus_one);
							CHILD(1).number() /= mstruct.number().denominator();
						} else {
							MathStructure mtest(CHILD(1));
							b = mtest.calculateRaise(mstruct.number().numerator(), eo);
							if(b && mtest.isPower() && mtest[1] == mstruct.number().numerator()) b = false;
							if(!b) break;
							set(mtest, true);
							raise(m_one);
							CHILD(1).number() /= mstruct.number().denominator();
						}
						if(b) calculateRaiseExponent(eo);
						return 1;
					}
					bool b;
					if(mstruct.number().isNegative()) {
						b = calculateRaise(-mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_minus_one);
						CHILD(1).number() /= mstruct.number().denominator();
					} else {
						b = calculateRaise(mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_one);
						CHILD(1).number() /= mstruct.number().denominator();
					}
					if(b) calculateRaiseExponent(eo);
					return 1;
				}
			}
			break;
		}
	}

	return -1;
}

int MathStructure::merge_logical_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {
	if(equals(mstruct, true, true)) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.representsNonZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.isZero()) {
		if(isZero()) return 2;
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(representsNonZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}

	if(CALCULATOR->aborted()) return -1;

	if(eo.test_comparisons && isLogicalOr()) {
		if(SIZE > 50) return -1;
		if(mstruct.isLogicalOr()) {
			if(mstruct.size() * SIZE > 50) return -1;
			for(size_t i = 0; i < SIZE; ) {
				MathStructure msave(CHILD(i));
				for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
					if(i2 > 0) {
						insertChild(msave, i + 1);
					}
					CHILD(i).calculateLogicalAnd(mstruct[i2], eo, this, i);
					i++;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).calculateLogicalAnd(mstruct, eo, this, i);
			}
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	} else if(eo.test_comparisons && mstruct.isLogicalOr()) {
		return 0;
	} else if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_NOT_EQUAL: {
					if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_EQUALS) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_NOT_EQUALS) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					} else if(ct_comp == COMPARISON_NOT_EQUALS && ct2 == COMPARISON_EQUALS) {
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
						} else {
							set_nocopy(mstruct, true);
						}
						return 3;
					}
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(ct_comp == ct2) return 1;
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_LESS;
							if(ct2 == COMPARISON_LESS) return 3;
							return 1;
						} else if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_GREATER;
							if(ct2 == COMPARISON_GREATER) return 3;
							return 1;
						}
					} else if(ct2 == COMPARISON_NOT_EQUALS) {
						if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
							if(ct_comp == COMPARISON_LESS) return 2;
							ct_comp = COMPARISON_LESS;
							return 1;
						} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
							if(ct_comp == COMPARISON_GREATER) return 2;
							ct_comp = COMPARISON_GREATER;
							return 1;
						}
					} else if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS) && (ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS)) {
						if(ct_comp == COMPARISON_EQUALS) return 2;
						ct_comp = COMPARISON_EQUALS;
						if(ct2 == COMPARISON_EQUALS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) && (ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS)) {
						if(ct_comp == COMPARISON_LESS) return 2;
						ct_comp = COMPARISON_LESS;
						if(ct2 == COMPARISON_LESS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) && (ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER)) {
						if(ct_comp == COMPARISON_GREATER) return 2;
						ct_comp = COMPARISON_GREATER;
						if(ct2 == COMPARISON_GREATER) return 3;
						return 1;
					}
					clear(true);
					return 1;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
					}
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
					}
					break;
				}
				default: {
					if(!eo.test_comparisons) return -1;
					if(comparisonType() == COMPARISON_EQUALS && !CHILD(1).contains(mstruct[0])) {
						mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
						if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
						mstruct.ref();
						add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
						calculateLogicalAndLast(eo);
						return 1;
					} else if(mstruct.comparisonType() == COMPARISON_EQUALS && !mstruct[1].contains(CHILD(0))) {
						calculateReplace(mstruct[0], mstruct[1], eo);
						if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
						mstruct.ref();
						add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
						calculateLogicalAndLast(eo);
						return 1;
					}
					return -1;
				}
			}
		} else if(eo.test_comparisons && comparisonType() == COMPARISON_EQUALS && !CHILD(0).isNumber() && !CHILD(0).containsInterval() && CHILD(1).isNumber() && mstruct.contains(CHILD(0))) {
			mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
			if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
			calculateLogicalAndLast(eo);
			return 1;
		} else if(eo.test_comparisons && mstruct.comparisonType() == COMPARISON_EQUALS && !mstruct[0].isNumber() && !mstruct[0].containsInterval() && mstruct[1].isNumber() && contains(mstruct[0])) {
			calculateReplace(mstruct[0], mstruct[1], eo);
			if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
			calculateLogicalAndLast(eo);
			return 1;
		}
	} else if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		}
		return 1;
	} else if(mstruct.isLogicalAnd()) {
		transform(STRUCT_LOGICAL_AND);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(mstruct.representsNonZero()) {
		if(isOne()) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(mstruct.isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(representsNonZero()) {
		if(!isOne()) set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(isZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(equals(mstruct, true, true)) {
		return 2;
	}
	if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			if(SIZE < mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < SIZE; i++) {
					b = false;
					for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
						if(CHILD(i) == mstruct[i2]) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(SIZE > mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					b = false;
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(mstruct[i] == CHILD(i2)) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i) == mstruct) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		}
	} else if(mstruct.isLogicalAnd()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(equals(mstruct[i])) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			}
		}
	}

	if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_NOT_EQUAL: {
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					if(ct_comp == ct2) return 1;
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_LESS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_GREATER: {}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_LESS: {}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
					}
					break;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						default: {}
					}
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
					}
					break;
				}
				default: {
					return -1;
				}
			}
		} else if(eo.test_comparisons && comparisonType() == COMPARISON_NOT_EQUALS && !CHILD(0).isNumber() && !CHILD(0).containsInterval() && CHILD(1).isNumber() && mstruct.contains(CHILD(0))) {
			mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
			if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_OR);
			calculateLogicalOrLast(eo);
			return 1;
		} else if(eo.test_comparisons && mstruct.comparisonType() == COMPARISON_NOT_EQUALS && !mstruct[0].isNumber() && !mstruct[0].containsInterval() && mstruct[1].isNumber() && contains(mstruct[0])) {
			calculateReplace(mstruct[0], mstruct[1], eo);
			if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_OR);
			calculateLogicalOrLast(eo);
			return 1;
		}
	} else if(isLogicalOr()) {
		if(mstruct.isLogicalOr()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		}
		return 1;
	} else if(mstruct.isLogicalOr()) {
		transform(STRUCT_LOGICAL_OR);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {

	if(equals(mstruct)) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bp1 = representsNonZero();
	bool bp2 = mstruct.representsNonZero();
	if(bp1 && bp2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bn1 = isZero();
	bool bn2 = mstruct.isZero();
	if(bn1 && bn2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	if((bn1 && bp2) || (bp1 && bn2)) {
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	return -1;

	/*int b0, b1;
	if(isZero()) {
		b0 = 0;
	} else if(representsNonZero(true)) {
		b0 = 1;
	} else {
		b0 = -1;
	}
	if(mstruct.isZero()) {
		b1 = 0;
	} else if(mstruct.representsNonZero(true)) {
		b1 = 1;
	} else {
		b1 = -1;
	}

	if((b0 == 1 && b1 == 0) || (b0 == 0 && b1 == 1)) {
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b0 >= 0 && b1 >= 0) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b0 == 0) {
		set(mstruct, true);
		add(m_zero, OPERATION_NOT_EQUALS);
		calculatesub(eo, eo, false);
		return 1;
	} else if(b0 == 1) {
		set(mstruct, true);
		add(m_zero, OPERATION_EQUALS);
		calculatesub(eo, eo, false);
		return 1;
	} else if(b1 == 0) {
		add(m_zero, OPERATION_NOT_EQUALS);
		calculatesub(eo, eo, false);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b1 == 1) {
		add(m_zero, OPERATION_EQUALS);
		calculatesub(eo, eo, false);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	MathStructure *mstruct2 = new MathStructure(*this);
	add(mstruct, OPERATION_LOGICAL_AND);
	LAST.calculateLogicalNot(eo);
	LAST.calculatesub(eo, eo, false);
	calculatesub(eo, eo, false);
	mstruct2->setLogicalNot();
	mstruct2->calculatesub(eo, eo, false);
	mstruct2->add(mstruct, OPERATION_LOGICAL_AND);
	mstruct2->calculatesub(eo, eo, false);
	add_nocopy(mstruct2, OPERATION_LOGICAL_OR);
	calculatesub(eo, eo, false);

	return 1;*/

}


int MathStructure::merge_bitwise_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitAnd(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_AND);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_AND: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_AND: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_AND: {
					return 0;
				}
				default: {}
			}
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitOr(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_OR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_OR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_OR: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_OR: {
					return 0;
				}
				default: {}
			}
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitXor(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_XOR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		default: {}
	}
	return -1;
}

#define MERGE_RECURSE			if(recursive) {\
						for(size_t i = 0; i < SIZE; i++) {\
							if(CALCULATOR->aborted()) break;\
							if(!CHILD(i).isNumber()) CHILD(i).calculatesub(eo, feo, true, this, i);\
						}\
						CHILDREN_UPDATED;\
					}

#define MERGE_ALL(FUNC, TRY_LABEL) 	size_t i2, i3 = SIZE;\
					bool do_abort = false; \
					for(size_t i = 0; i < SIZE - 1; i++) {\
						i2 = i + 1;\
						TRY_LABEL:\
						for(; i2 < i; i2++) {\
							if(CALCULATOR->aborted()) break;\
							int r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i);\
							if(r == 0) {\
								SWAP_CHILDREN(i2, i);\
								r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i2, i);\
								}\
							}\
							if(r >= 1) {\
								ERASE(i);\
								b = true;\
								i3 = i;\
								i = i2;\
								i2 = 0;\
								goto TRY_LABEL;\
							}\
						}\
						for(i2 = i + 1; i2 < SIZE; i2++) {\
							if(CALCULATOR->aborted()) break;\
							int r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2);\
							if(r == 0) {\
								SWAP_CHILDREN(i, i2);\
								r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i, i2);\
								} else if(r == 2) {\
									r = 3;\
								} else if(r == 3) {\
									r = 2;\
								}\
							}\
							if(r >= 1) {\
								ERASE(i2);\
								b = true;\
								if(r != 2) {\
									i2 = 0;\
									goto TRY_LABEL;\
								}\
								i2--;\
							} else if(CHILD(i).isDateTime()) {\
								do_abort = true;\
								break;\
							}\
						}\
						if(do_abort) break;\
						if(i3 < SIZE) {\
							if(i3 == SIZE - 1) break;\
							i = i3;\
							i3 = SIZE;\
							i2 = i + 1;\
							goto TRY_LABEL;\
						}\
					}

#define MERGE_ALL2			if(SIZE == 1) {\
						setToChild(1, false, mparent, index_this + 1);\
					} else if(SIZE == 0) {\
						clear(true);\
					} else {\
						evalSort();\
					}

bool fix_intervals(MathStructure &mstruct, const EvaluationOptions &eo, bool *failed, long int min_precision, bool function_middle) {
	if(mstruct.type() == STRUCT_NUMBER) {
		if(eo.interval_calculation != INTERVAL_CALCULATION_NONE) {
			if(!mstruct.number().isInterval(false) && mstruct.number().precision() >= 0 && (CALCULATOR->usesIntervalArithmetic() || mstruct.number().precision() <= PRECISION + 10)) {
				mstruct.number().precisionToInterval();
				mstruct.setPrecision(-1);
				mstruct.numberUpdated();
				return true;
			}
		} else if(mstruct.number().isInterval(false)) {
			if(!mstruct.number().intervalToPrecision(min_precision)) {
				if(failed) *failed = true;
				return false;
			}
			mstruct.numberUpdated();
			return true;
		}
	} else if(mstruct.type() == STRUCT_FUNCTION && (mstruct.function()->id() == FUNCTION_ID_INTERVAL || mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY)) {
		if(eo.interval_calculation == INTERVAL_CALCULATION_NONE) {
			bool b = mstruct.calculateFunctions(eo, false);
			if(b) {
				fix_intervals(mstruct, eo, failed, function_middle);
				return true;
			} else if(function_middle && mstruct.type() == STRUCT_FUNCTION && mstruct.function()->id() == FUNCTION_ID_INTERVAL && mstruct.size() == 2) {
				mstruct.setType(STRUCT_ADDITION);
				mstruct.divide(nr_two);
				return true;
			} else if(function_middle && mstruct.type() == STRUCT_FUNCTION && mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY && mstruct.size() >= 1) {
				mstruct.setToChild(1, true);
				return true;
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(fix_intervals(mstruct[i], eo, failed, function_middle)) {
				mstruct.childUpdated(i + 1);
				b = true;
			}
		}
		return b;
	}
	return false;
}

bool contains_zero_unit(const MathStructure &mstruct);
bool contains_zero_unit(const MathStructure &mstruct) {
	if(mstruct.isMultiplication() && mstruct.size() > 1 && mstruct[0].isZero()) {
		bool b = true;
		for(size_t i = 1; i < mstruct.size(); i++) {
			if(!mstruct[i].isUnit_exp()) {
				b = false;
				break;
			}
		}
		if(b) return true;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_zero_unit(mstruct[i])) return true;
	}
	return false;
}

bool test_var_int(const MathStructure &mstruct, bool *v = NULL) {
	if(mstruct.isVariable() && (mstruct.variable()->id() == VARIABLE_ID_E || mstruct.variable()->id() == VARIABLE_ID_PI)) {
		if(!v) return true;
		if(*v) return false;
		else *v = true;
		return true;
	}
	if(mstruct.isNumber() && mstruct.number().isReal()) {
		if(!v) {
			if(mstruct.number().isInterval()) {
				Number nr_int(mstruct.number());
				nr_int.round();
				return mstruct.number() < nr_int || mstruct.number() > nr_int;
			}
			if(mstruct.isApproximate()) {
				Number nr_f = mstruct.number();
				nr_f.floor();
				Number nr_c(nr_f);
				nr_c++;
				return COMPARISON_IS_NOT_EQUAL(mstruct.number().compareApproximately(nr_f)) && COMPARISON_IS_NOT_EQUAL(mstruct.number().compareApproximately(nr_c));
			}
			return !mstruct.number().isInterval() && !mstruct.number().isInteger();
		}
		if(mstruct.isApproximate()) return false;
		return mstruct.number().isRational();
	}
	if(mstruct.isMultiplication() || mstruct.isAddition() || (mstruct.isPower() && mstruct[1].isInteger())) {
		bool v2 = false;
		if(!v) v = &v2;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!test_var_int(mstruct[i], v)) return false;
		}
		if(*v) return true;
	}
	return false;
}

bool test_non_integer(const MathStructure &mstruct, const EvaluationOptions&) {
	if(test_var_int(mstruct)) return true;
	if(!mstruct.isApproximate()) {
		if((mstruct.isMultiplication() || mstruct.isAddition()) && mstruct.size() >= 2 && mstruct[0].isNumber() && mstruct[0].number().isReal() && !mstruct[0].number().isInterval() && !mstruct[0].number().isInteger()) {
			for(size_t i = 1; i < mstruct.size(); i++) {
				if(!mstruct[i].representsInteger()) return false;
			}
			return true;
		}
	}
	return false;
}
bool MathStructure::factorizeUnits() {
	switch(m_type) {
		case STRUCT_ADDITION: {
			if(containsType(STRUCT_DATETIME, false, true, false) > 0) return false;
			bool b = false;
			MathStructure mstruct_units(*this);
			MathStructure mstruct_new(*this);
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(CALCULATOR->aborted()) break;
				if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
						if(CALCULATOR->aborted()) break;
						if(!mstruct_units[i][i2].isUnit_exp()) {
							mstruct_units[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
					else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
					for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
						if(CALCULATOR->aborted()) break;
						if(mstruct_new[i][i2].isUnit_exp()) {
							mstruct_new[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
					else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
				} else if(mstruct_units[i].isUnit_exp()) {
					mstruct_new[i].set(1, 1, 0);
				} else {
					mstruct_units[i].setUndefined();
				}
			}
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(CALCULATOR->aborted()) break;
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
				else set(mstruct_new, true);
				return true;
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).factorizeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
	}
}

bool MathStructure::calculatesub(const EvaluationOptions &eo, const EvaluationOptions &feo, bool recursive, MathStructure *mparent, size_t index_this) {
	if(b_protected) return false;
	bool b = false;

	switch(m_type) {
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if((eo.approximation == APPROXIMATION_APPROXIMATE || (!o_variable->isApproximate() && !((KnownVariable*) o_variable)->get().containsInterval(true, false, false, 0, true))) && !((KnownVariable*) o_variable)->get().isAborted()) {
					set(((KnownVariable*) o_variable)->get());
					unformat(eo);
					if(eo.calculate_functions) {
						calculateFunctions(feo);
					}
					fix_intervals(*this, feo, NULL, PRECISION);
					b = true;
					calculatesub(eo, feo, true, mparent, index_this);
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_power(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);
			}
			break;
		}
		case STRUCT_ADDITION: {
			MERGE_RECURSE
			bool found_nonlinear_relations = false;
			if(eo.sync_units && (syncUnits(false, &found_nonlinear_relations, true, feo) || (found_nonlinear_relations && eo.sync_nonlinear_unit_relations))) {
				if(found_nonlinear_relations && eo.sync_nonlinear_unit_relations) {
					EvaluationOptions eo2 = eo;
					eo2.expand = -3;
					eo2.combine_divisions = false;
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculatesub(eo2, feo, true, this, i);
						CHILD(i).factorizeUnits();
					}
					CHILDREN_UPDATED;
					syncUnits(true, NULL, true, feo);
				}
				unformat(eo);
				MERGE_RECURSE
			}
			MERGE_ALL(merge_addition, try_add)
			MERGE_ALL2
			break;
		}
		case STRUCT_MULTIPLICATION: {

			MERGE_RECURSE
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				MERGE_RECURSE
			}

			if(representsNonMatrix()) {
				if(SIZE > 2) {
					int nonintervals = 0, had_interval = false;
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isNumber()) {
							if(CHILD(i).number().isInterval(false)) {
								had_interval = true;
								if(nonintervals >= 2) break;
							} else if(nonintervals < 2) {
								nonintervals++;
								if(nonintervals == 2 && had_interval) break;
							}
						}
					}
					if(had_interval && nonintervals >= 2) evalSort(false);
				}

				MERGE_ALL(merge_multiplication, try_multiply)

			} else {
				size_t i2, i3 = SIZE;
				for(size_t i = 0; i < SIZE - 1; i++) {
					i2 = i + 1;
					try_multiply_matrix:
					bool b_matrix = !CHILD(i).representsNonMatrix();
					if(i2 < i) {
						for(; ; i2--) {
							int r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i);
							if(r == 0) {
								SWAP_CHILDREN(i2, i);
								r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i, true);
								if(r < 1) {
									SWAP_CHILDREN(i2, i);
								}
							}
							if(r >= 1) {
								ERASE(i);
								b = true;
								i3 = i;
								i = i2;
								i2 = 0;
								goto try_multiply_matrix;
							}
							if(i2 == 0) break;
							if(b_matrix && !CHILD(i2).representsNonMatrix()) break;
						}
					}
					bool had_matrix = false;
					for(i2 = i + 1; i2 < SIZE; i2++) {
						if(had_matrix && !CHILD(i2).representsNonMatrix()) continue;
						int r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2);
						if(r == 0) {
							SWAP_CHILDREN(i, i2);
							r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2, true);
							if(r < 1) {
								SWAP_CHILDREN(i, i2);
							} else if(r == 2) {
								r = 3;
							} else if(r == 3) {
								r = 2;
							}
						}
						if(r >= 1) {
							ERASE(i2);
							b = true;
							if(r != 2) {
								i2 = 0;
								goto try_multiply_matrix;
							}
							i2--;
						}
						if(i == SIZE - 1) break;
						if(b_matrix && !CHILD(i2).representsNonMatrix()) had_matrix = true;
					}
					if(i3 < SIZE) {
						if(i3 == SIZE - 1) break;
						i = i3;
						i3 = SIZE;
						i2 = i + 1;
						goto try_multiply_matrix;
					}
				}
			}

			MERGE_ALL2

			break;
		}
		case STRUCT_BITWISE_AND: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_and, try_bitand)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_OR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_or, try_bitor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_XOR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_xor, try_bitxor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			switch(CHILD(0).type()) {
				case STRUCT_NUMBER: {
					Number nr(CHILD(0).number());
					if(nr.bitNot() && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || CHILD(0).number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || CHILD(0).number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || CHILD(0).number().includesInfinity())) {
						set(nr, true);
					}
					break;
				}
				case STRUCT_VECTOR: {
					SET_CHILD_MAP(0);
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).setLogicalNot();
					}
					break;
				}
				case STRUCT_BITWISE_NOT: {
					setToChild(1);
					setToChild(1);
					break;
				}
				default: {}
			}
			break;
		}
		case STRUCT_LOGICAL_AND: {
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).isZero()) {
						clear(true);
						b = true;
						break;
					}
				}
				if(b) break;
			}
			MERGE_ALL(merge_logical_and, try_logand)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsNonZero()) {
					set(1, 1, 0, true);
				} else if(CHILD(0).isZero()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_NOT_EQUALS;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
			bool isResistance = false;
			switch(CHILD(0).type()) {
				case STRUCT_MULTIPLICATION: {
					if(CHILD(0)[1] != 0 && CHILD(0)[1].unit() && CHILD(0)[1].unit()->name().find("ohm") != string::npos) {
						isResistance = true;
					}
					break;
				}
				case STRUCT_UNIT: {
					if (CHILD(0).unit() && CHILD(0).unit()->name().find("ohm") != string::npos) {
						isResistance = true;
					}
					break;
				}
				default: {}
			}

			if(isResistance) {
				MathStructure mstruct;
				for (size_t i = 0; i < SIZE; i++) {
					MathStructure mtemp(CHILD(i));
					mtemp.inverse();
					mstruct += mtemp;
				}
				mstruct.inverse();
				clear();
				set(mstruct);
				break;
			}
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).representsNonZero()) {
						set(1, 1, 0, true);
						b = true;
						break;
					}
				}
				if(b) break;
			}
			MERGE_ALL(merge_logical_or, try_logor)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsNonZero()) {
					set(1, 1, 0, true);
				} else if(CHILD(0).isZero()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_NOT_EQUALS;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_logical_xor(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).representsNonZero()) {
				clear(true);
				b = true;
			} else if(CHILD(0).isZero()) {
				set(1, 1, 0, true);
				b = true;
			} else if(CHILD(0).isLogicalNot()) {
				setToChild(1);
				setToChild(1);
				if(!representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					add(m_zero, OPERATION_NOT_EQUALS);
					calculatesub(eo, feo, false);
				}
				b = true;
			}
			break;
		}
		case STRUCT_COMPARISON: {
			EvaluationOptions eo2 = eo;
			if(eo2.assume_denominators_nonzero == 1) eo2.assume_denominators_nonzero = false;
			if(recursive) {
				CHILD(0).calculatesub(eo2, feo, true, this, 0);
				CHILD(1).calculatesub(eo2, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				if(recursive) {
					CHILD(0).calculatesub(eo2, feo, true, this, 0);
					CHILD(1).calculatesub(eo2, feo, true, this, 1);
					CHILDREN_UPDATED;
				}
			}
			if(CHILD(0).isAddition() || CHILD(1).isAddition()) {
				size_t i2 = 0;
				for(size_t i = 0; !CHILD(0).isAddition() || i < CHILD(0).size(); i++) {
					if(CHILD(1).isAddition()) {
						for(; i2 < CHILD(1).size(); i2++) {
							if(CHILD(0).isAddition() && CHILD(0)[i] == CHILD(1)[i2]) {
								CHILD(0).delChild(i + 1);
								CHILD(1).delChild(i2 + 1);
								break;
							} else if(!CHILD(0).isAddition() && CHILD(0) == CHILD(1)[i2]) {
								CHILD(0).clear(true);
								CHILD(1).delChild(i2 + 1);
								break;
							}
						}
					} else if(CHILD(0)[i] == CHILD(1)) {
						CHILD(1).clear(true);
						CHILD(0).delChild(i + 1);
						break;
					}
					if(!CHILD(0).isAddition()) break;
				}
				if(CHILD(0).isAddition()) {
					if(CHILD(0).size() == 1) CHILD(0).setToChild(1, true);
					else if(CHILD(0).size() == 0) CHILD(0).clear(true);
				}
				if(CHILD(1).isAddition()) {
					if(CHILD(1).size() == 1) CHILD(1).setToChild(1, true);
					else if(CHILD(1).size() == 0) CHILD(1).clear(true);
				}
			}
			if(CHILD(0).isMultiplication() && CHILD(1).isMultiplication()) {
				size_t i1 = 0, i2 = 0;
				if(CHILD(0)[0].isNumber()) i1++;
				if(CHILD(1)[0].isNumber()) i2++;
				while(i1 < CHILD(0).size() && i2 < CHILD(1).size()) {
					if(CHILD(0)[i1] == CHILD(1)[i2] && CHILD(0)[i1].representsPositive(true)) {
						CHILD(0).delChild(i1 + 1);
						CHILD(1).delChild(i2 + 1);
					} else {
						break;
					}
				}
				if(CHILD(0).size() == 1) CHILD(0).setToChild(1, true);
				else if(CHILD(0).size() == 0) CHILD(0).set(1, 1, 0, true);
				if(CHILD(1).size() == 1) CHILD(1).setToChild(1, true);
				else if(CHILD(1).size() == 0) CHILD(1).set(1, 1, 0, true);
			}
			if(((CHILD(0).isNumber() || (CHILD(0).isVariable() && !CHILD(0).variable()->isKnown() && ((UnknownVariable*) CHILD(0).variable())->interval().isNumber())) && (CHILD(1).isNumber() || ((CHILD(1).isVariable() && !CHILD(1).variable()->isKnown() && ((UnknownVariable*) CHILD(1).variable())->interval().isNumber())))) || (CHILD(0).isDateTime() && CHILD(1).isDateTime())) {
				ComparisonResult cr;
				if(CHILD(0).isNumber()) {
					if(CHILD(1).isNumber()) cr = CHILD(1).number().compareApproximately(CHILD(0).number());
					else cr = ((UnknownVariable*) CHILD(1).variable())->interval().number().compareApproximately(CHILD(0).number());
				} else if(CHILD(1).isNumber()) {
					cr = CHILD(1).number().compareApproximately(((UnknownVariable*) CHILD(0).variable())->interval().number());
				} else if(CHILD(1).isVariable()) {
					cr = ((UnknownVariable*) CHILD(1).variable())->interval().number().compareApproximately(((UnknownVariable*) CHILD(0).variable())->interval().number());
				} else {
					cr = CHILD(1).compare(CHILD(0));
				}
				if(cr >= COMPARISON_RESULT_UNKNOWN) {
					break;
				}
				switch(ct_comp) {
					case COMPARISON_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							set(1, 1, 0, true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_NOT_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							clear(true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							set(1, 1, 0, true);
							b = true;
						}
						break;
					}
					case COMPARISON_LESS: {
						if(cr == COMPARISON_RESULT_LESS) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_GREATER: {
						if(cr == COMPARISON_RESULT_GREATER) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
				}
				break;
			}
			if(!eo.test_comparisons) {
				break;
			}
			if(eo2.keep_zero_units && contains_zero_unit(*this)) {
				eo2.keep_zero_units = false;
				MathStructure mtest(*this);
				CALCULATOR->beginTemporaryStopMessages();
				mtest.calculatesub(eo2, feo, true);
				if(mtest.isNumber()) {
					CALCULATOR->endTemporaryStopMessages(true);
					set(mtest);
					b = true;
					break;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
			if((CHILD(0).representsUndefined() && !CHILD(1).representsUndefined(true, true, true)) || (CHILD(1).representsUndefined() && !CHILD(0).representsUndefined(true, true, true))) {
				if(ct_comp == COMPARISON_EQUALS) {
					clear(true);
					b = true;
					break;
				} else if(ct_comp == COMPARISON_NOT_EQUALS) {
					set(1, 1, 0, true);
					b = true;
					break;
				}
			}
			if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_GREATER) && CHILD(1).isZero()) {
				if(CHILD(0).isLogicalNot() || CHILD(0).isLogicalAnd() || CHILD(0).isLogicalOr() || CHILD(0).isLogicalXor() || CHILD(0).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_LESS) {
						ERASE(1);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(1, false, mparent, index_this + 1);
					}
					b = true;
				}
			} else if((ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_LESS) && CHILD(0).isZero()) {
				if(CHILD(1).isLogicalNot() || CHILD(1).isLogicalAnd() || CHILD(1).isLogicalOr() || CHILD(1).isLogicalXor() || CHILD(1).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_GREATER) {
						ERASE(0);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(2, false, mparent, index_this + 1);
					}
					b = true;
				}
			}
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				if((CHILD(0).representsReal(true) && CHILD(1).representsComplex(true)) || (CHILD(1).representsReal(true) && CHILD(0).representsComplex(true))) {
					if(ct_comp == COMPARISON_EQUALS) {
						clear(true);
					} else {
						set(1, 1, 0, true);
					}
					b = true;
				} else if((CHILD(0).representsZero(true) && CHILD(1).representsZero(true))) {
					if(ct_comp != COMPARISON_EQUALS) {
						clear(true);
					} else {
						set(1, 1, 0, true);
					}
					b = true;
				} else if(CHILD(0).isVariable() && !CHILD(0).variable()->isKnown() && CHILD(0).representsInteger() && test_non_integer(CHILD(1), eo)) {
					if(ct_comp == COMPARISON_EQUALS) clear(true);
					else set(1, 1, 0, true);
					b = true;
				}
			}
			if(b) break;
			if(CHILD(1).isNumber() && CHILD(0).isVariable() && !CHILD(0).variable()->isKnown()) {
				Assumptions *ass = ((UnknownVariable*) CHILD(0).variable())->assumptions();
				if(ass && ass->min()) {
					bool b_inc = ass->includeEqualsMin();
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_LESS: {
							if(CHILD(1).number() <= *ass->min()) {clear(true); b = true;}

							break;
						}
						case COMPARISON_GREATER: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(b_inc && CHILD(1).number() == *ass->min()) {ct_comp = COMPARISON_EQUALS; b = true;}
							else if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(CHILD(1).number() <= *ass->min()) {set(1, 1, 0, true); b = true;}
							break;
						}
					}
				}
				if(ass && ass->max() && isComparison()) {
					bool b_inc = ass->includeEqualsMax();
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_LESS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_GREATER: {
							if(CHILD(1).number() >= *ass->max()) {clear(true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(CHILD(1).number() >= *ass->max()) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(b_inc && CHILD(1).number() == *ass->max()) {ct_comp = COMPARISON_EQUALS; b = true;}
							else if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {clear(true); b = true;}
							break;
						}
					}
				}
			}
			if(b) break;
			if(eo.approximation == APPROXIMATION_EXACT && eo.test_comparisons > 0) {
				bool b_intval = CALCULATOR->usesIntervalArithmetic();
				bool b_failed = false;
				EvaluationOptions eo3 = feo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo3.approximation = APPROXIMATION_APPROXIMATE;
				eo2.test_comparisons = false;
				MathStructure mtest(*this);
				for(int i = 0; i < 2; i++) {
					CALCULATOR->beginTemporaryEnableIntervalArithmetic();
					int b_ass = (i == 0 ? 2 : contains_ass_intval(mtest));
					if(b_ass == 0 || !CALCULATOR->usesIntervalArithmetic()) {CALCULATOR->endTemporaryEnableIntervalArithmetic(); break;}
					replace_interval_unknowns(mtest, i > 0);
					if(i == 0 && !b_intval) fix_intervals(mtest, eo2, &b_failed);
					if(!b_failed) {
						if(i == 0 && mtest[0].isAddition() && mtest[0].size() > 1 && mtest[1].isZero()) {
							mtest[1] = mtest[0][0];
							mtest[1].negate();
							mtest[0].delChild(1, true);
						}
						CALCULATOR->beginTemporaryStopMessages();
						if(b_ass == 2) mtest[0].calculateFunctions(eo3);
						mtest[0].calculatesub(eo2, eo3, true);
						if(b_ass == 2) mtest[1].calculateFunctions(eo3);
						mtest[1].calculatesub(eo2, eo3, true);
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						mtest.childrenUpdated();
						if(CALCULATOR->endTemporaryStopMessages(NULL, NULL, MESSAGE_ERROR) == 0) {
							eo2.approximation = eo.approximation;
							eo2.test_comparisons = -1;
							mtest.calculatesub(eo2, feo, false);
							if(mtest.isNumber()) {
								if(mtest.isZero()) clear(true);
								else set(1, 1, 0, true);
								b = true;
								return b;
							}
						}
					} else {
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						break;
					}
				}
			}
			eo2 = eo;
			if(eo2.assume_denominators_nonzero == 1) eo2.assume_denominators_nonzero = false;
			bool mtest_new = false;
			MathStructure *mtest;
			if(!CHILD(1).isZero()) {
				if(!eo.isolate_x || find_x_var().isUndefined()) {
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
					mtest = &CHILD(0);
					mtest->ref();
				} else {
					mtest = new MathStructure(CHILD(0));
					mtest->calculateSubtract(CHILD(1), eo2);
					remove_rad_unit(*mtest, eo2);
					mtest_new = true;
				}
			} else {
				mtest = &CHILD(0);
				mtest->ref();
			}
			int incomp = 0;
			if(mtest->isAddition()) {
				mtest->evalSort(true);
				incomp = compare_check_incompability(mtest);
				if(incomp == 1 && !mtest_new) {
					mtest->unref();
					mtest = new MathStructure(CHILD(0));
					if(remove_rad_unit(*mtest, eo2)) {
						mtest_new = true;
						if(mtest->isAddition()) {
							mtest->evalSort(true);
							incomp = compare_check_incompability(mtest);
						}
					}
				}
			}
			if(incomp <= 0) {
				if(mtest_new && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
					bool a_pos = CHILD(0).representsPositive(true);
					bool a_nneg = a_pos || CHILD(0).representsNonNegative(true);
					bool a_neg = !a_nneg && CHILD(0).representsNegative(true);
					bool a_npos = !a_pos && (a_neg || CHILD(0).representsNonPositive(true));
					bool b_pos = CHILD(1).representsPositive(true);
					bool b_nneg = b_pos || CHILD(1).representsNonNegative(true);
					bool b_neg = !b_nneg && CHILD(1).representsNegative(true);
					bool b_npos = !b_pos && (b_neg || CHILD(1).representsNonPositive(true));
					if(isApproximate()) {
						if((a_pos && b_neg) || (a_neg && b_pos)) {
							incomp = 1;
						}
					} else {
						if((a_pos && b_npos) || (a_npos && b_pos) || (a_nneg && b_neg) || (a_neg && b_nneg)) {
							incomp = 1;
						}
					}
				} else if(incomp < 0) {
					mtest->unref();
					break;
				}
			}

			switch(ct_comp) {
				case COMPARISON_EQUALS: {
					if(incomp > 0) {
						clear(true);
						b = true;
					} else if(mtest->representsZero(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_NOT_EQUALS: {
					if(incomp > 0) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonZero(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_LESS: {
					if(incomp > 0) {
					} else if(mtest->representsNegative(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_GREATER: {
					if(incomp > 0) {
					} else if(mtest->representsPositive(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_LESS: {
					if(incomp > 0) {
					} else if(mtest->representsNonPositive(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_GREATER: {
					if(incomp > 0) {
					} else if(mtest->representsNonNegative(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
			}
			mtest->unref();
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_ABS || o_function->id() == FUNCTION_ID_ROOT || o_function->id() == FUNCTION_ID_INTERVAL || o_function->id() == FUNCTION_ID_UNCERTAINTY || o_function->id() == FUNCTION_ID_SIGNUM || o_function->id() == FUNCTION_ID_DIRAC || o_function->id() == FUNCTION_ID_HEAVISIDE) {
				b = calculateFunctions(eo, false);
				if(b) {
					calculatesub(eo, feo, true, mparent, index_this);
					break;
				}
			} else if(o_function->id() == FUNCTION_ID_STRIP_UNITS) {
				b = calculateFunctions(eo, false);
				if(b) calculatesub(eo, feo, true, mparent, index_this);
				break;
			}
		}
		default: {
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).calculatesub(eo, feo, true, this, i)) b = true;
				}
				CHILDREN_UPDATED;
			}
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				if(recursive) {
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).calculatesub(eo, feo, true, this, i)) b = true;
					}
					CHILDREN_UPDATED;
				}
			}
		}
	}
	return b;
}

#define MERGE_INDEX(FUNC, TRY_LABEL)	bool b = false;\
					TRY_LABEL:\
					for(size_t i = 0; i < index; i++) {\
						if(CALCULATOR->aborted()) break; \
						int r = CHILD(i).FUNC(CHILD(index), eo, this, i, index);\
						if(r == 0) {\
							SWAP_CHILDREN(i, index);\
							r = CHILD(i).FUNC(CHILD(index), eo, this, i, index, true);\
							if(r < 1) {\
								SWAP_CHILDREN(i, index);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(index);\
							if(!b && r == 2) {\
								b = true;\
								index = SIZE;\
								break;\
							} else {\
								b = true;\
								index = i;\
								goto TRY_LABEL;\
							}\
						}\
					}\
					for(size_t i = index + 1; i < SIZE; i++) {\
						if(CALCULATOR->aborted()) break; \
						int r = CHILD(index).FUNC(CHILD(i), eo, this, index, i);\
						if(r == 0) {\
							SWAP_CHILDREN(index, i);\
							r = CHILD(index).FUNC(CHILD(i), eo, this, index, i, true);\
							if(r < 1) {\
								SWAP_CHILDREN(index, i);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(i);\
							if(!b && r == 3) {\
								b = true;\
								break;\
							}\
							b = true;\
							if(r != 2) {\
								goto TRY_LABEL;\
							}\
							i--;\
						}\
					}

#define MERGE_INDEX2			if(b && check_size) {\
						if(SIZE == 1) {\
							setToChild(1, false, mparent, index_this + 1);\
						} else if(SIZE == 0) {\
							clear(true);\
						} else {\
							evalSort();\
						}\
						return true;\
					} else {\
						evalSort();\
						return b;\
					}


bool MathStructure::calculateMergeIndex(size_t index, const EvaluationOptions &eo, const EvaluationOptions &feo, MathStructure *mparent, size_t index_this) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			return calculateMultiplyIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_ADDITION: {
			return calculateAddIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_POWER: {
			return calculateRaiseExponent(eo, mparent, index_this);
		}
		case STRUCT_LOGICAL_AND: {
			return calculateLogicalAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_OR: {
			return calculateLogicalOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_XOR: {
			return calculateLogicalXorLast(eo, mparent, index_this);
		}
		case STRUCT_BITWISE_AND: {
			return calculateBitwiseAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_OR: {
			return calculateBitwiseOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_XOR: {
			return calculateBitwiseXorIndex(index, eo, true, mparent, index_this);
		}
		default: {}
	}
	return calculatesub(eo, feo, false, mparent, index_this);
}
bool MathStructure::calculateLogicalOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalOr()) {
		CALCULATOR->error(true, "calculateLogicalOrIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_or, try_logical_or_index)

	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}

}
bool MathStructure::calculateLogicalOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_LOGICAL_OR, true);
	LAST.evalSort();
	return calculateLogicalOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateLogicalXorLast(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {

	if(!isLogicalXor()) {
		CALCULATOR->error(true, "calculateLogicalXorLast() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_logical_xor(CHILD(1), eo, this, 0, 1) >= 1) {
		if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
			setToChild(1, false, mparent, index_this + 1);
		} else if(CHILD(0).representsPositive()) {
			clear(true);
			o_number.setTrue();
		} else if(CHILD(0).representsNonPositive()) {
			clear(true);
			o_number.setFalse();
		} else {
			APPEND(m_zero);
			m_type = STRUCT_COMPARISON;
			ct_comp = COMPARISON_GREATER;
		}
		return true;
	}
	return false;

}
bool MathStructure::calculateLogicalXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_LOGICAL_XOR);
	LAST.evalSort();
	return calculateLogicalXorLast(eo, mparent, index_this);
}
bool MathStructure::calculateLogicalAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalAnd()) {
		CALCULATOR->error(true, "calculateLogicalAndIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_and, try_logical_and_index)

	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}

}
bool MathStructure::calculateLogicalAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_LOGICAL_AND, true);
	LAST.evalSort();
	return calculateLogicalAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateInverse(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	return calculateRaise(m_minus_one, eo, mparent, index_this);
}
bool MathStructure::calculateNegate(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.negate() && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
		if(!isMultiplication()) transform(STRUCT_MULTIPLICATION);
		PREPEND(m_minus_one);
		return false;
	}
	if(!isMultiplication()) transform(STRUCT_MULTIPLICATION);
	PREPEND(m_minus_one);
	return calculateMultiplyIndex(0, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_LOGICAL_NOT);
	return calculatesub(eo, eo, false, mparent, index_this);
}
bool MathStructure::calculateLogicalNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_BITWISE_NOT);
	return calculatesub(eo, eo, false, mparent, index_this);
}
bool MathStructure::calculateRaiseExponent(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(!isPower()) {
		CALCULATOR->error(true, "calculateRaiseExponent() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_power(CHILD(1), eo, this, 0, 1) >= 1) {
		setToChild(1, false, mparent, index_this + 1);
		return true;
	}
	return false;
}
bool MathStructure::calculateRaise(const MathStructure &mexp, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mexp.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.raise(mexp.number(), eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mexp.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mexp.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mexp.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	raise(mexp);
	LAST.evalSort();
	return calculateRaiseExponent(eo, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseAnd()) {
		CALCULATOR->error(true, "calculateBitwiseAndIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_and, try_bitwise_and_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_BITWISE_AND, true);
	LAST.evalSort();
	return calculateBitwiseAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseOr()) {
		CALCULATOR->error(true, "calculateBitwiseOrIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_or, try_bitwise_or_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_BITWISE_OR, true);
	LAST.evalSort();
	return calculateBitwiseOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseXorIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseXor()) {
		CALCULATOR->error(true, "calculateBitwiseXorIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_xor, try_bitwise_xor_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_BITWISE_XOR, true);
	LAST.evalSort();
	return calculateBitwiseXorIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateMultiplyLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateMultiplyIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateMultiplyIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isMultiplication()) {
		CALCULATOR->error(true, "calculateMultiplyIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	bool b = false;
	try_multiply_matrix_index:
	bool b_matrix = !CHILD(index).representsNonMatrix();
	if(index > 0) {
		for(size_t i = index - 1; ; i--) {
			if(CALCULATOR->aborted()) break;
			int r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index);
			if(r == 0) {
				SWAP_CHILDREN(i, index);
				r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index, true);
				if(r < 1) {
					SWAP_CHILDREN(i, index);
				} else if(r == 2) {
					r = 3;
				} else if(r == 3) {
					r = 2;
				}
			}
			if(r >= 1) {
				ERASE(index);
				if(!b && r == 2) {
					b = true;
					index = SIZE;
					break;
				} else {
					b = true;
					index = i;
					goto try_multiply_matrix_index;
				}
			}
			if(i == 0) break;
			if(b_matrix && !CHILD(i).representsNonMatrix()) break;
		}
	}

	bool had_matrix = false;
	for(size_t i = index + 1; i < SIZE; i++) {
		if(had_matrix && !CHILD(i).representsNonMatrix()) continue;
		if(CALCULATOR->aborted()) break;
		int r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i);
		if(r == 0) {
			SWAP_CHILDREN(index, i);
			r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i, true);
			if(r < 1) {
				SWAP_CHILDREN(index, i);
			} else if(r == 2) {
				r = 3;
			} else if(r == 3) {
				r = 2;
			}
		}
		if(r >= 1) {
			ERASE(i);
			if(!b && r == 3) {
				b = true;
				break;
			}
			b = true;
			if(r != 2) {
				goto try_multiply_matrix_index;
			}
			i--;
		}
		if(i == SIZE - 1) break;
		if(b_matrix && !CHILD(i).representsNonMatrix()) had_matrix = true;
	}

	MERGE_INDEX2

}
bool MathStructure::calculateMultiply(const MathStructure &mmul, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mmul.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.multiply(mmul.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mmul.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mmul.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mmul.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	multiply(mmul, true);
	LAST.evalSort();
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateDivide(const MathStructure &mdiv, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mdiv.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.divide(mdiv.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mdiv.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mdiv.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mdiv.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	MathStructure *mmul = new MathStructure(mdiv);
	mmul->evalSort();
	multiply_nocopy(mmul, true);
	LAST.calculateInverse(eo, this, SIZE - 1);
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateAddLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateAddIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateAddIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isAddition()) {
		CALCULATOR->error(true, "calculateAddIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_addition, try_add_index)
	MERGE_INDEX2

}
bool MathStructure::calculateAdd(const MathStructure &madd, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(madd.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.add(madd.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || madd.number().isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	add(madd, true);
	LAST.evalSort();
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateSubtract(const MathStructure &msub, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(msub.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.subtract(msub.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || msub.number().isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	MathStructure *madd = new MathStructure(msub);
	madd->evalSort();
	add_nocopy(madd, true);
	LAST.calculateNegate(eo, this, SIZE - 1);
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}

bool MathStructure::calculateFunctions(const EvaluationOptions &eo, bool recursive, bool do_unformat) {

	if(m_type == STRUCT_FUNCTION && o_function != eo.protected_function) {

		if(function_value) {
			function_value->unref();
			function_value = NULL;
		}

		if(!o_function->testArgumentCount(SIZE)) {
			return false;
		}

		if(o_function->maxargs() > -1 && (long int) SIZE > o_function->maxargs()) {
			if(o_function->maxargs() == 1 && o_function->getArgumentDefinition(1) && o_function->getArgumentDefinition(1)->handlesVector()) {
				bool b = false;
				for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
					CHILD(0)[i2].transform(o_function);
					if(CHILD(0)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
					CHILD(0).childUpdated(i2 + 1);
				}
				SET_CHILD_MAP(0)
				return b;
			}
			REDUCE(o_function->maxargs());
		}
		m_type = STRUCT_VECTOR;
		Argument *arg = NULL, *last_arg = NULL;
		int last_i = 0;

		for(size_t i = 0; i < SIZE; i++) {
			arg = o_function->getArgumentDefinition(i + 1);
			if(arg) {
				last_arg = arg;
				last_i = i;
				if(i > 0 && arg->type() == ARGUMENT_TYPE_SYMBOLIC && (CHILD(i).isZero() || CHILD(i).isUndefined())) {
					CHILD(i).set(CHILD(0).find_x_var(), true);
					if(CHILD(i).isUndefined() && CHILD(0).isVariable() && CHILD(0).variable()->isKnown()) CHILD(i).set(((KnownVariable*) CHILD(0).variable())->get().find_x_var(), true);
					if(CHILD(i).isUndefined()) {
						CALCULATOR->beginTemporaryStopMessages();
						MathStructure mtest(CHILD(0));
						mtest.eval(eo);
						CHILD(i).set(mtest.find_x_var(), true);
						CALCULATOR->endTemporaryStopMessages();
					}
					if(CHILD(i).isUndefined()) {
						CALCULATOR->error(false, _("No unknown variable/symbol was found."), NULL);
						CHILD(i).set(CALCULATOR->getVariableById(VARIABLE_ID_X), true);
					}
				}
				if(!arg->test(CHILD(i), i + 1, o_function, eo)) {
					if(arg->handlesVector() && CHILD(i).isVector()) {
						bool b = false;
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							CHILD(i)[i2].transform(o_function);
							for(size_t i3 = 0; i3 < SIZE; i3++) {
								if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
								else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
							}
							if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
							CHILD(i).childUpdated(i2 + 1);
						}
						SET_CHILD_MAP(i);
						return b;
					}
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
				if(arg->handlesVector()) {
					if(arg->tests() && !CHILD(i).isVector() && !CHILD(i).representsScalar()) {
						CHILD(i).eval(eo);
						CHILD_UPDATED(i);
					}
					if(CHILD(i).isVector()) {
						bool b = false;
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							CHILD(i)[i2].transform(o_function);
							for(size_t i3 = 0; i3 < SIZE; i3++) {
								if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
								else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
							}
							if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
							CHILD(i).childUpdated(i2 + 1);
						}
						SET_CHILD_MAP(i);
						return b;
					}
				}
			}
		}

		if(last_arg && o_function->maxargs() < 0 && last_i >= o_function->minargs()) {
			for(size_t i = last_i + 1; i < SIZE; i++) {
				if(!last_arg->test(CHILD(i), i + 1, o_function, eo)) {
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
			}
		}

		if(!o_function->testCondition(*this)) {
			m_type = STRUCT_FUNCTION;
			return false;
		}
		MathStructure *mstruct = new MathStructure();
		int ret = o_function->calculate(*mstruct, *this, eo);
		if(ret > 0) {
			set_nocopy(*mstruct, true);
			if(recursive) calculateFunctions(eo);
			mstruct->unref();
			if(do_unformat) unformat(eo);
			return true;
		} else {
			if(ret < 0) {
				ret = -ret;
				if(o_function->maxargs() > 0 && ret > o_function->maxargs()) {
					if(mstruct->isVector()) {
						if(do_unformat) mstruct->unformat(eo);
						for(size_t arg_i = 1; arg_i <= SIZE && arg_i <= mstruct->size(); arg_i++) {
							mstruct->getChild(arg_i)->ref();
							setChild_nocopy(mstruct->getChild(arg_i), arg_i);
						}
					}
				} else if(ret <= (long int) SIZE) {
					if(do_unformat) mstruct->unformat(eo);
					mstruct->ref();
					setChild_nocopy(mstruct, ret);
				}
			}
			/*if(eo.approximation == APPROXIMATION_EXACT) {
				mstruct->clear();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				CALCULATOR->beginTemporaryStopMessages();
				if(o_function->calculate(*mstruct, *this, eo2) > 0) {
					function_value = mstruct;
					function_value->ref();
					function_value->calculateFunctions(eo2);
				}
				if(CALCULATOR->endTemporaryStopMessages() > 0 && function_value) {
					function_value->unref();
					function_value = NULL;
				}
			}*/
			m_type = STRUCT_FUNCTION;
			mstruct->unref();
			for(size_t i = 0; i < SIZE; i++) {
				arg = o_function->getArgumentDefinition(i + 1);
				if(arg && arg->handlesVector()) {
					if(!CHILD(i).isVector()) {
						CHILD(i).calculatesub(eo, eo, false);
						if(!CHILD(i).isVector()) return false;
					}
					bool b = false;
					for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
						CHILD(i)[i2].transform(o_function);
						for(size_t i3 = 0; i3 < SIZE; i3++) {
							if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
							else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
						}
						if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
						CHILD(i).childUpdated(i2 + 1);
					}
					SET_CHILD_MAP(i);
					return b;
				}
			}
			return false;
		}
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(CHILD(i).calculateFunctions(eo, recursive, do_unformat)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
	}
	return b;

}

int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, bool b_abs = false);
int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, bool b_abs) {
	if(parent.isMultiplication()) {
		if((!mstruct1.representsNonMatrix() && !mstruct2.representsScalar()) || (!mstruct2.representsNonMatrix() && !mstruct1.representsScalar())) {
			return 0;
		}
	}
	if(b_abs || parent.isAddition()) {
		if(mstruct1.isMultiplication() && mstruct1.size() > 0) {
			size_t start = 0;
			while(mstruct1[start].isNumber() && mstruct1.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct2.isMultiplication()) {
				if(mstruct2.size() < 1) return -1;
				size_t start2 = 0;
				while(mstruct2[start2].isNumber() && mstruct2.size() > start2 + 1) {
					start2++;
				}
				for(size_t i = 0; ; i++) {
					if(i + start2 >= mstruct2.size()) {
						if(i + start >= mstruct1.size()) {
							if(start2 == start) {
								for(size_t i3 = 0; i3 < start; i3++) {
									i2 = evalSortCompare(mstruct1[i3], mstruct2[i3], parent, b_abs);
									if(i2 != 0) return i2;
								}
								return 0;
							}
							if(start2 > start) return -1;
						}
						return 1;
					}
					if(i + start >= mstruct1.size()) return -1;
					i2 = evalSortCompare(mstruct1[i + start], mstruct2[i + start2], parent, b_abs);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				return -1;
			} else {
				i2 = evalSortCompare(mstruct1[start], mstruct2, parent, b_abs);
				if(i2 != 0) return i2;
				if(b_abs && start == 1 && (mstruct1[0].isMinusOne() || mstruct1[0].isOne())) return 0;
				return 1;
			}
		} else if(mstruct2.isMultiplication() && mstruct2.size() > 0) {
			size_t start = 0;
			while(mstruct2[start].isNumber() && mstruct2.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct1.isMultiplication()) {
				return 1;
			} else {
				i2 = evalSortCompare(mstruct1, mstruct2[start], parent, b_abs);
				if(i2 != 0) return i2;
				if(b_abs && start == 1 && (mstruct2[0].isMinusOne() || mstruct2[0].isOne())) return 0;
				return -1;
			}
		}
	}
	if(mstruct1.type() != mstruct2.type()) {
		if(!parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			if(mstruct2.isPower()) {
				int i = evalSortCompare(mstruct1, mstruct2[0], parent, b_abs);
				if(i == 0) {
					return evalSortCompare(m_one, mstruct2[1], parent, b_abs);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				int i = evalSortCompare(mstruct1[0], mstruct2, parent, b_abs);
				if(i == 0) {
					return evalSortCompare(mstruct1[1], m_one, parent, b_abs);
				}
				return i;
			}
		}
		if(mstruct2.isAborted()) return -1;
		if(mstruct1.isAborted()) return 1;
		if(mstruct2.isInverse()) return -1;
		if(mstruct1.isInverse()) return 1;
		if(mstruct2.isDivision()) return -1;
		if(mstruct1.isDivision()) return 1;
		if(mstruct2.isNegate()) return -1;
		if(mstruct1.isNegate()) return 1;
		if(mstruct2.isLogicalAnd()) return -1;
		if(mstruct1.isLogicalAnd()) return 1;
		if(mstruct2.isLogicalOr()) return -1;
		if(mstruct1.isLogicalOr()) return 1;
		if(mstruct2.isLogicalXor()) return -1;
		if(mstruct1.isLogicalXor()) return 1;
		if(mstruct2.isLogicalNot()) return -1;
		if(mstruct1.isLogicalNot()) return 1;
		if(mstruct2.isComparison()) return -1;
		if(mstruct1.isComparison()) return 1;
		if(mstruct2.isBitwiseOr()) return -1;
		if(mstruct1.isBitwiseOr()) return 1;
		if(mstruct2.isBitwiseXor()) return -1;
		if(mstruct1.isBitwiseXor()) return 1;
		if(mstruct2.isBitwiseAnd()) return -1;
		if(mstruct1.isBitwiseAnd()) return 1;
		if(mstruct2.isBitwiseNot()) return -1;
		if(mstruct1.isBitwiseNot()) return 1;
		if(mstruct2.isUndefined()) return -1;
		if(mstruct1.isUndefined()) return 1;
		if(mstruct2.isFunction()) return -1;
		if(mstruct1.isFunction()) return 1;
		if(mstruct2.isAddition()) return -1;
		if(mstruct1.isAddition()) return 1;
		if(mstruct2.isMultiplication()) return -1;
		if(mstruct1.isMultiplication()) return 1;
		if(mstruct2.isPower()) return -1;
		if(mstruct1.isPower()) return 1;
		if(mstruct2.isUnit()) return -1;
		if(mstruct1.isUnit()) return 1;
		if(mstruct2.isSymbolic()) return -1;
		if(mstruct1.isSymbolic()) return 1;
		if(mstruct2.isVariable()) return -1;
		if(mstruct1.isVariable()) return 1;
		if(mstruct2.isDateTime()) return -1;
		if(mstruct1.isDateTime()) return 1;
		if(parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			if(CALCULATOR->aborted()) return 0;
			if(b_abs) {
				ComparisonResult cmp = mstruct1.number().compareAbsolute(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			}
			if(!mstruct1.number().hasImaginaryPart() && !mstruct2.number().hasImaginaryPart()) {
				if(mstruct1.number().isFloatingPoint()) {
					if(!mstruct2.number().isFloatingPoint()) return 1;
					if(mstruct1.number().isInterval()) {
						if(!mstruct2.number().isInterval()) return 1;
					} else if(mstruct2.number().isInterval()) return -1;
				} else if(mstruct2.number().isFloatingPoint()) return -1;
				ComparisonResult cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						return 1;
					} else {
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					}
					if(cmp == COMPARISON_RESULT_LESS) return -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_UNIT: {
			if(mstruct1.unit() < mstruct2.unit()) return -1;
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() < mstruct2.variable()) return -1;
			else if(mstruct1.variable() == mstruct2.variable()) return 0;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() < mstruct2.function()) return -1;
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						return -1;
					}
					int i2 = evalSortCompare(mstruct1[i], mstruct2[i], parent, b_abs);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			return 1;
		}
		case STRUCT_POWER: {
			int i = evalSortCompare(mstruct1[0], mstruct2[0], parent, b_abs);
			if(i == 0) {
				return evalSortCompare(mstruct1[1], mstruct2[1], parent, b_abs);
			}
			return i;
		}
		default: {
			if(mstruct2.size() < mstruct1.size()) return -1;
			else if(mstruct2.size() > mstruct1.size()) return 1;
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				ie = evalSortCompare(mstruct1[i], mstruct2[i], parent, b_abs);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::evalSort(bool recursive, bool b_abs) {
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).evalSort(true, b_abs);
		}
	}
	//if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR && m_type != STRUCT_LOGICAL_XOR && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return;
	vector<size_t> sorted;
	sorted.reserve(SIZE);
	for(size_t i = 0; i < SIZE; i++) {
		if(i == 0) {
			sorted.push_back(v_order[i]);
		} else {
			if(evalSortCompare(CHILD(i), *v_subs[sorted.back()], *this, b_abs) >= 0) {
				sorted.push_back(v_order[i]);
			} else if(sorted.size() == 1) {
				sorted.insert(sorted.begin(), v_order[i]);
			} else {
				for(size_t i2 = sorted.size() - 2; ; i2--) {
					if(evalSortCompare(CHILD(i), *v_subs[sorted[i2]], *this, b_abs) >= 0) {
						sorted.insert(sorted.begin() + i2 + 1, v_order[i]);
						break;
					}
					if(i2 == 0) {
						sorted.insert(sorted.begin(), v_order[i]);
						break;
					}
				}
			}
		}
	}
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
}


