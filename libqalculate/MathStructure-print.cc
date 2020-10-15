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

#include <map>
#include <algorithm>

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::ostream;
using std::endl;

void printRecursive(const MathStructure &mstruct) {
	std::cout << "RECURSIVE " << mstruct.print() << std::endl;
	for(size_t i = 0; i < mstruct.size(); i++) {
		std::cout << i << ": " << mstruct[i].print() << std::endl;
		for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
			std::cout << i << "-" << i2 << ": " << mstruct[i][i2].print() << std::endl;
			for(size_t i3 = 0; i3 < mstruct[i][i2].size(); i3++) {
				std::cout << i << "-" << i2 << "-" << i3 << ": " << mstruct[i][i2][i3].print() << std::endl;
				for(size_t i4 = 0; i4 < mstruct[i][i2][i3].size(); i4++) {
					std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << ": " << mstruct[i][i2][i3][i4].print() << std::endl;
					for(size_t i5 = 0; i5 < mstruct[i][i2][i3][i4].size(); i5++) {
						std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << ": " << mstruct[i][i2][i3][i4][i5].print() << std::endl;
						for(size_t i6 = 0; i6 < mstruct[i][i2][i3][i4][i5].size(); i6++) {
							std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << ": " << mstruct[i][i2][i3][i4][i5][i6].print() << std::endl;
							for(size_t i7 = 0; i7 < mstruct[i][i2][i3][i4][i5][i6].size(); i7++) {
								std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7].print() << std::endl;
								for(size_t i8 = 0; i8 < mstruct[i][i2][i3][i4][i5][i6][i7].size(); i8++) {
									std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << "-" << i8 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7][i8].print() << std::endl;
									for(size_t i9 = 0; i9 < mstruct[i][i2][i3][i4][i5][i6][i7][i8].size(); i9++) {
										std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << "-" << i8 << "-" << i9 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7][i8][i9].print() << std::endl;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

string format_and_print(const MathStructure &mstruct) {
	MathStructure m_print(mstruct);
	if(CALCULATOR) {
		m_print.sort(CALCULATOR->messagePrintOptions());
		m_print.formatsub(CALCULATOR->messagePrintOptions(), NULL, 0, true, &m_print);
		return m_print.print(CALCULATOR->messagePrintOptions());
	} else {
		PrintOptions po;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		po.spell_out_logical_operators = true;
		po.number_fraction_format = FRACTION_FRACTIONAL;
		m_print.sort(po);
		m_print.formatsub(po, NULL, 0, true, &m_print);
		return m_print.print(po);
	}
}

bool name_is_less(const string &str1, const string &str2) {
	for(size_t i = 0; ; i++) {
		if(i == str1.length()) return true;
		if(i == str2.length()) return false;
		char c1 = str1[i];
		char c2 = str2[i];
		if(c1 < 0 || c2 < 0) break;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 'a' - 'A';
		if(c2 >= 'A' && c2 <= 'Z') c2 += 'a' - 'A';
		if(c1 < c2) return true;
		if(c1 > c2) return false;
	}
	char *s1 = utf8_strdown(str1.c_str());
	char *s2 = utf8_strdown(str2.c_str());
	if(s1 && s2) {
		bool b = strcmp(s1, s2) < 0;
		free(s1);
		free(s2);
		return b;
	}
	return false;
}

int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po);
int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po) {
	// returns -1 if mstruct1 should be placed before mstruct2, 1 if mstruct1 should be placed after mstruct2, and 0 if current order should be preserved
	if(parent.isMultiplication()) {
		if(!mstruct1.representsNonMatrix() && !mstruct2.representsNonMatrix()) {
			// the order of matrices should be preserved
			return 0;
		}
		if(mstruct1.isNumber() && mstruct2.isPower() && mstruct2[0].isInteger() && mstruct2[1].isInteger() && mstruct2[1].representsPositive()) {
			return sortCompare(mstruct1, mstruct2[0], parent, po);
		}
		if(mstruct2.isNumber() && mstruct1.isPower() && mstruct1[0].isInteger() && mstruct1[1].isInteger() && mstruct1[1].representsPositive()) {
			return sortCompare(mstruct1[0], mstruct2, parent, po);
		}
		if(mstruct1.isPower() && mstruct2.isPower() && mstruct1[0].isInteger() && mstruct1[1].isInteger() && mstruct1[1].representsPositive() && mstruct2[0].isInteger() && mstruct2[1].isInteger() && mstruct2[1].representsPositive()) {
			return sortCompare(mstruct1[0], mstruct2[0], parent, po);
		}
	}
	if(parent.isAddition()) {

		// always place constant of definite integral last
		if(mstruct1.isVariable() && mstruct1.variable() == CALCULATOR->getVariableById(VARIABLE_ID_C)) return 1;
		if(mstruct2.isVariable() && mstruct2.variable() == CALCULATOR->getVariableById(VARIABLE_ID_C)) return -1;

		if(po.sort_options.minus_last) {
			// -a+b=b-a
			bool m1 = mstruct1.hasNegativeSign(), m2 = mstruct2.hasNegativeSign();
			if(m1 && !m2) {
				return 1;
			} else if(m2 && !m1) {
				return -1;
			}
		}

		if((mstruct1.isUnit() || (mstruct1.isMultiplication() && mstruct1.size() == 2 && mstruct1[1].isUnit())) && (mstruct2.isUnit() || (mstruct2.isMultiplication() && mstruct2.size() == 2 && mstruct2[1].isUnit()))) {
			Unit *u1, *u2;
			if(mstruct1.isUnit()) u1 = mstruct1.unit();
			else u1 = mstruct1[1].unit();
			if(mstruct2.isUnit()) u2 = mstruct2.unit();
			else u2 = mstruct2[1].unit();
			if(u1->isParentOf(u2)) return 1;
			if(u2->isParentOf(u1)) return -1;
		}
	}
	bool isdiv1 = false, isdiv2 = false;
	if(!po.negative_exponents || !mstruct1.isUnit_exp() || !mstruct2.isUnit_exp()) {
		// determine if mstruct1 and/or mstruct2 is division (a*b^-1)
		if(mstruct1.isMultiplication()) {
			for(size_t i = 0; i < mstruct1.size(); i++) {
				if(mstruct1[i].isPower() && mstruct1[i][1].hasNegativeSign()) {
					isdiv1 = true;
					break;
				}
			}
		} else if(mstruct1.isPower() && mstruct1[1].hasNegativeSign()) {
			isdiv1 = true;
		}
		if(mstruct2.isMultiplication()) {
			for(size_t i = 0; i < mstruct2.size(); i++) {
				if(mstruct2[i].isPower() && mstruct2[i][1].hasNegativeSign()) {
					isdiv2 = true;
					break;
				}
			}
		} else if(mstruct2.isPower() && mstruct2[1].hasNegativeSign()) {
			isdiv2 = true;
		}
	}
	if(parent.isAddition() && isdiv1 == isdiv2) {
		// sort using single factors from left to right
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
				for(size_t i = 0; i + start < mstruct1.size(); i++) {
					if(i + start2 >= mstruct2.size()) return 1;
					i2 = sortCompare(mstruct1[i + start], mstruct2[i + start2], parent, po);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				if(parent.isMultiplication()) return -1;
				else return 1;
			} else {
				i2 = sortCompare(mstruct1[start], mstruct2, parent, po);
				if(i2 != 0) return i2;
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
				i2 = sortCompare(mstruct1, mstruct2[start], parent, po);
				if(i2 != 0) return i2;
			}
		}
	}
	if(mstruct1.type() != mstruct2.type()) {
		if(mstruct1.isVariable() && mstruct2.isSymbolic()) {
			if(parent.isMultiplication()) {
				// place constant (known) factors first (before symbols)
				if(mstruct1.variable()->isKnown()) return -1;
			}
			// sort variables and symbols in alphabetical order
			if(name_is_less(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name, mstruct2.symbol())) return -1;
			else return 1;
		}
		if(mstruct2.isVariable() && mstruct1.isSymbolic()) {
			if(parent.isMultiplication()) {
				// place constant (known) factors first (before symbols)
				if(mstruct2.variable()->isKnown()) return 1;
			}
			// sort variables and symbols in alphabetical order
			if(name_is_less(mstruct1.symbol(), mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name)) return -1;
			else return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			// sort exponentiation with non-exponentiation (has exponent 1), excluding number factors
			if(mstruct2.isPower()) {
				if(mstruct2[0].isUnit() && mstruct1.isUnit()) return -1;
				int i = sortCompare(mstruct1, mstruct2[0], parent, po);
				if(i == 0) {
					return sortCompare(m_one, mstruct2[1], parent, po);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				if(mstruct1[0].isUnit() && mstruct2.isUnit()) return 1;
				int i = sortCompare(mstruct1[0], mstruct2, parent, po);
				if(i == 0) {
					return sortCompare(mstruct1[1], m_one, parent, po);
				}
				return i;
			}
		}
		if(parent.isMultiplication()) {
			// place unit factors last
			if(mstruct2.isUnit()) return -1;
			if(mstruct1.isUnit()) return 1;
			if(mstruct1.isAddition() && !mstruct2.isAddition() && !mstruct1.containsUnknowns() && (mstruct2.isUnknown_exp() || (mstruct2.isMultiplication() && mstruct2.containsUnknowns()))) return -1;
			if(mstruct2.isAddition() && !mstruct1.isAddition() && !mstruct2.containsUnknowns() && (mstruct1.isUnknown_exp() || (mstruct1.isMultiplication() && mstruct1.containsUnknowns()))) return 1;
		}
		// types listed in sort order (aborted is placed last)
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
		if(!parent.isMultiplication()) {
			// place division last
			if(isdiv2 && mstruct2.isMultiplication()) return -1;
			if(isdiv1 && mstruct1.isMultiplication()) return 1;
			// place number after multiplication, exponentiation, unit, symbol, variable and date/time
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
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
			// only reached when type of 1 or 2 is a vector (place number factor after vector)
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			bool inc_order = parent.isMultiplication();
			if(!mstruct1.number().hasImaginaryPart() && !mstruct2.number().hasImaginaryPart()) {
				// real numbers
				ComparisonResult cmp;
				// if the numbers are factors with different signs, place smallest number first (the one with the negative sign)
				if(parent.isMultiplication() && mstruct2.number().isNegative() != mstruct1.number().isNegative()) cmp = mstruct2.number().compare(mstruct1.number());
				// otherwise, place largest number first
				else cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return inc_order ? 1 : -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return inc_order ? -1 : 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						// place number with real part before number without
						return 1;
					} else {
						// compare imaginary parts
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return inc_order ? 1 : -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return inc_order ? -1 : 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					// compare real parts first
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						// if real parts are equal, compare imaginary parts
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					}
					if(cmp == COMPARISON_RESULT_LESS) return inc_order ? 1 : -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return inc_order ? -1 : 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_UNIT: {
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			// sort units in alphabetical order
			if(name_is_less(mstruct1.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct1.isPlural(), po.use_reference_names).name, mstruct2.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct2.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name)) return -1;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			// sort symbols in alphabetical order
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() == mstruct2.variable()) return 0;
			if(parent.isMultiplication()) {
				// place constant (known) factors first (before unknown variables)
				if(mstruct1.variable()->isKnown() && !mstruct2.variable()->isKnown()) return -1;
				if(!mstruct1.variable()->isKnown() && mstruct2.variable()->isKnown()) return 1;
			}
			// sort variables in alphabetical order
			if(name_is_less(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name, mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name)) return -1;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						// place function with less arguments first (if common arguments are equal)
						return -1;
					}
					// sort same functions using arguments
					int i2 = sortCompare(mstruct1[i], mstruct2[i], parent, po);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			// sort functions in alphabetical order
			if(name_is_less(mstruct1.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name, mstruct2.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name)) return -1;
			return 1;
		}
		case STRUCT_POWER: {
			if(parent.isMultiplication() && mstruct1[0].isUnit() && mstruct2[0].isUnit()) {
				int i = sortCompare(mstruct1[1], mstruct2[1], parent, po);
				if(i == 0) {
					return sortCompare(mstruct1[0], mstruct2[0], parent, po);
				}
				return i;
			}
			// compare bases first
			int i = sortCompare(mstruct1[0], mstruct2[0], parent, po);
			if(i == 0) {
				// compare exponents if bases are equal
				return sortCompare(mstruct1[1], mstruct2[1], parent, po);
			}
			return i;
		}
		case STRUCT_MULTIPLICATION: {
			if(isdiv1 != isdiv2) {
				// place denominator last
				if(isdiv1) return 1;
				return -1;
			}
		}
		case STRUCT_COMPARISON: {
			if(mstruct1.comparisonType() != mstruct2.comparisonType()) {
				// place equality before inequality, place greater before less
				if(mstruct2.comparisonType() == COMPARISON_EQUALS || ((mstruct1.comparisonType() == COMPARISON_LESS || mstruct1.comparisonType() == COMPARISON_EQUALS_LESS) && (mstruct2.comparisonType() == COMPARISON_GREATER || mstruct2.comparisonType() == COMPARISON_EQUALS_GREATER))) {
					return 1;
				}
				if(mstruct1.comparisonType() == COMPARISON_EQUALS || ((mstruct1.comparisonType() == COMPARISON_GREATER || mstruct1.comparisonType() == COMPARISON_EQUALS_GREATER) && (mstruct2.comparisonType() == COMPARISON_LESS || mstruct2.comparisonType() == COMPARISON_EQUALS_LESS))) {
					return -1;
				}
			}
		}
		default: {
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				// place MathStructure with less children first (if common children are equal)
				if(i >= mstruct2.size()) return 1;
				// sort by comparing children
				ie = sortCompare(mstruct1[i], mstruct2[i], parent, po);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::sort(const PrintOptions &po, bool recursive) {
	// sort before output
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).sort(po);
		}
	}
	if(m_type == STRUCT_COMPARISON) {
		// place zero and numbers last in comparison
		if((CHILD(0).isZero() && !CHILD(1).isZero()) || (CHILD(0).isNumber() && !CHILD(1).isNumber())) {
			SWAP_CHILDREN(0, 1)
			if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_GREATER;
			else if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS_GREATER;
			else if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_LESS;
			else if(ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = COMPARISON_EQUALS_LESS;
		}
		return;
	}
	// only sort addition, multiplication, bitwise AND, OR, XOR, and logical AND, OR
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR && m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR) return;
	// do not sort addition containing date/time
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return;
	vector<size_t> sorted;
	bool b;
	PrintOptions po2 = po;
	po2.sort_options.minus_last = po.sort_options.minus_last && SIZE == 2;
	for(size_t i = 0; i < SIZE; i++) {
		if(CALCULATOR->aborted()) return;
		b = false;
		for(size_t i2 = 0; i2 < sorted.size(); i2++) {
			if(sortCompare(CHILD(i), *v_subs[sorted[i2]], *this, po2) < 0) {
				sorted.insert(sorted.begin() + i2, v_order[i]);
				b = true;
				break;
			}
		}
		if(!b) sorted.push_back(v_order[i]);
	}
	if(CALCULATOR->aborted()) return;
	if(m_type == STRUCT_ADDITION && SIZE > 2 && po.sort_options.minus_last && v_subs[sorted[0]]->hasNegativeSign()) {
		for(size_t i2 = 1; i2 < sorted.size(); i2++) {
			if(CALCULATOR->aborted()) return;
			if(!v_subs[sorted[i2]]->hasNegativeSign()) {
				sorted.insert(sorted.begin(), sorted[i2]);
				sorted.erase(sorted.begin() + (i2 + 1));
				break;
			}
		}
	}
	if(CALCULATOR->aborted()) return;
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
}

void MathStructure::unformat(const EvaluationOptions &eo) {
	if(m_type == STRUCT_FUNCTION && o_function->id() == FUNCTION_ID_STRIP_UNITS) {
		EvaluationOptions eo2 = eo;
		eo2.keep_prefixes = true;
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).unformat(eo2);
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).unformat(eo);
		}
	}
	switch(m_type) {
		case STRUCT_INVERSE: {
			APPEND(m_minus_one);
			m_type = STRUCT_POWER;
			break;
		}
		case STRUCT_NEGATE: {
			PREPEND(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
			break;
		}
		case STRUCT_DIVISION: {
			CHILD(1).raise(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
			break;
		}
		case STRUCT_UNIT: {
			if(o_prefix && !eo.keep_prefixes) {
				if(o_prefix == CALCULATOR->getDecimalNullPrefix() || o_prefix == CALCULATOR->getBinaryNullPrefix()) {
					o_prefix = NULL;
				} else {
					Unit *u = o_unit;
					Prefix *p = o_prefix;
					set(p->value());
					multiply(u);
				}
				unformat(eo);
				break;
			} else if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				set(((CompositeUnit*) o_unit)->generateMathStructure(false, eo.keep_prefixes));
				unformat(eo);
				break;
			}
			b_plural = false;
			break;
		}
		default: {}
	}
}

void idm1(const MathStructure &mnum, bool &bfrac, bool &bint) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if((!bfrac || bint) && mnum.number().isRational()) {
				if(!mnum.number().isInteger()) {
					bint = false;
					bfrac = true;
				}
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if((!bfrac || bint) && mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(!mnum[0].number().isInteger()) {
					bint = false;
					bfrac = true;
				}

			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_ADDITION: {
			bool b_a = false;
			for(size_t i = 0; i < mnum.size() && (!bfrac || bint); i++) {
				if(mnum[i].isAddition()) b_a = true;
				else idm1(mnum[i], bfrac, bint);
			}
			if(b_a) bint = false;
			break;
		}
		default: {
			bint = false;
		}
	}
}
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(mnum.number().isRational()) {
				if(mnum.number().isInteger()) {
					if(bint) {
						if(mnum.number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum.number();
						} else if(nr != mnum.number()) {
							nr.gcd(mnum.number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum.number().denominator();
					} else {
						Number nden(mnum.number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(mnum[0].number().isInteger()) {
					if(bint) {
						if(mnum[0].number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum[0].number();
						} else if(nr != mnum[0].number()) {
							nr.gcd(mnum[0].number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum[0].number().denominator();
					} else {
						Number nden(mnum[0].number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size() && (bfrac || bint); i++) {
				if(!mnum[i].isAddition()) idm2(mnum[i], bfrac, bint, nr);
			}
			break;
		}
		default: {}
	}
}
int idm3(MathStructure &mnum, Number &nr, bool expand) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			mnum.number() *= nr;
			mnum.numberUpdated();
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				mnum[0].number() *= nr;
				if(mnum[0].number().isOne() && mnum.size() != 1) {
					mnum.delChild(1, true);
				}
				return -1;
			} else if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					if(mnum[i].isAddition()) {
						idm3(mnum[i], nr, true);
						return -1;
					}
				}
			}
			mnum.insertChild(nr, 1);
			return 1;
		}
		case STRUCT_ADDITION: {
			if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					idm3(mnum[i], nr, true);
				}
				break;
			}
		}
		default: {
			mnum.transform(STRUCT_MULTIPLICATION);
			mnum.insertChild(nr, 1);
			return -1;
		}
	}
	return 0;
}

void idm1b(const MathStructure &mnum, bool &bint, bool &bint2) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(mnum.number().isInteger() && !mnum.number().isOne()) {
				bint = true;
				if(mnum.number() > 9 || mnum.number() < -9) bint2 = true;
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				idm1b(mnum[0], bint, bint2);
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				if(mnum[i].isAddition()) bint = false;
				else idm1b(mnum[i], bint, bint2);
				if(!bint) break;
			}
			break;
		}
		default: {
			bint = false;
		}
	}
}
void idm2b(const MathStructure &mnum, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(nr.isZero() || mnum.number() < nr) nr = mnum.number();
			break;
		}
		case STRUCT_MULTIPLICATION: {
			idm2b(mnum[0], nr);
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				idm2b(mnum[i], nr);
			}
			break;
		}
		default: {}
	}
}
void idm3b(MathStructure &mnum, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			mnum.number() /= nr;
			break;
		}
		case STRUCT_MULTIPLICATION: {
			idm3b(mnum[0], nr);
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				idm3b(mnum[i], nr);
			}
			break;
		}
		default: {}
	}
}

bool displays_number_exact(Number nr, const PrintOptions &po, MathStructure *top_parent) {
	if(po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) return true;
	InternalPrintStruct ips_n;
	if(top_parent && top_parent->isApproximate()) ips_n.parent_approximate = true;
	if(po.show_ending_zeroes && po.restrict_to_parent_precision && ips_n.parent_approximate && (nr > 9 || nr < -9)) return false;
	if(top_parent && top_parent->precision() < 0) ips_n.parent_precision = top_parent->precision();
	bool approximately_displayed = false;
	PrintOptions po2 = po;
	po2.indicate_infinite_series = false;
	po2.is_approximate = &approximately_displayed;
	nr.print(po2, ips_n);
	return !approximately_displayed;
}

bool fix_approximate_multiplier(MathStructure &m, const PrintOptions &po, MathStructure *top_parent = NULL) {
	if(!top_parent) top_parent = &m;
	if(po.number_fraction_format == FRACTION_DECIMAL) {
		PrintOptions po2 = po;
		po2.number_fraction_format = FRACTION_FRACTIONAL;
		po2.restrict_fraction_length = true;
		return fix_approximate_multiplier(m, po2, top_parent);
	}
	bool b_ret = false;
	if(m.isMultiplication() && m.size() >= 2 && m[0].isNumber() && m[0].number().isRational()) {
		for(size_t i = 1; i < m.size(); i++) {
			if(m[i].isAddition()) {
				bool mul_exact = displays_number_exact(m[0].number(), po, top_parent);
				bool b = false;
				for(size_t i2 = 0; i2 < m[i].size() && !b; i2++) {
					if(m[i][i2].isNumber() && (!mul_exact || !displays_number_exact(m[i][i2].number(), po, top_parent))) {
						b = true;
					} else if(m[i][i2].isMultiplication() && m[i][i2].size() >= 2 && m[i][i2][0].isNumber() && (!mul_exact || !displays_number_exact(m[i][i2][0].number(), po, top_parent))) {
						b = true;
					}
				}
				if(b) {
					for(size_t i2 = 0; i2 < m[i].size(); i2++) {
						if(m[i][i2].isNumber()) {
							if(!m[i][i2].number().multiply(m[0].number())) {
								m[i][i2].multiply(m[0]);
							}
						} else if(m[i][i2].isMultiplication()) {
							if(m[i][i2].size() < 1 || !m[i][i2][0].isNumber() || !m[i][i2][0].number().multiply(m[0].number())) {
								m[i][i2].insertChild(m[0], 1);
							}
						} else {
							m[i][i2].multiply(m[0]);
							m[i][i2].swapChildren(1, 2);
						}
					}
					m.delChild(1, true);
					b_ret = true;
					break;
				}
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_approximate_multiplier(m[i], po, top_parent)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	return b_ret;
}

int idm3_test(bool &b_fail, const MathStructure &mnum, const Number &nr, bool expand, const PrintOptions &po, MathStructure *top_parent) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			Number nr2(mnum.number());
			nr2 *= nr;
			b_fail = !displays_number_exact(nr2, po, top_parent);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				Number nr2(mnum[0].number());
				nr2 *= nr;
				b_fail = !nr2.isOne() && !displays_number_exact(nr2, po, top_parent);
				return -1;
			} else if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					if(mnum[i].isAddition()) {
						idm3_test(b_fail, mnum[i], nr, true, po, top_parent);
						return -1;
					}
				}
			}
			b_fail = !displays_number_exact(nr, po, top_parent);
			return 1;
		}
		case STRUCT_ADDITION: {
			if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					idm3_test(b_fail, mnum[i], nr, true, po, top_parent);
					if(b_fail) break;
				}
				break;
			}
		}
		default: {
			b_fail = !displays_number_exact(nr, po, top_parent);
			return -1;
		}
	}
	return 0;
}

bool is_unit_multiexp(const MathStructure &mstruct) {
	if(mstruct.isUnit_exp()) return true;
	if(mstruct.isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct.size(); i3++) {
			if(!mstruct[i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	} else if(mstruct.isDivision()) {
		return is_unit_multiexp(mstruct[0]) && is_unit_multiexp(mstruct[1]);
	} else if(mstruct.isInverse()) {
		return is_unit_multiexp(mstruct[0]);
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct[0].size(); i3++) {
			if(!mstruct[0][i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	}
	return false;
}

bool MathStructure::improve_division_multipliers(const PrintOptions &po, MathStructure *top_parent) {
	if(!top_parent) top_parent = this;
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			size_t inum = 0, iden = 0;
			bool bfrac = false, bint = true, bdiv = false, bnonunitdiv = false;
			size_t index1 = 0, index2 = 0;
			bool dofrac = true;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(iden == 0) index1 = i2;
						iden++;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
						if(CHILD(i2)[0].containsType(STRUCT_ADDITION)) {
							dofrac = true;
						}
					}
				} else if(!bdiv && CHILD(i2).isPower() && CHILD(i2)[1].hasNegativeSign()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(!bdiv) index1 = i2;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
					}
				} else {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2))) {
						if(inum == 0) index2 = i2;
						inum++;
					}
				}
			}
			if(!bdiv || !bnonunitdiv) break;
			if(iden > 1) {
				size_t i2 = index1 + 1;
				while(i2 < SIZE) {
					if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
						CHILD(index1)[0].multiply(CHILD(i2)[0], true);
						ERASE(i2);
						if(index2 > i2) index2--;
					} else {
						i2++;
					}
				}
				iden = 1;
			}
			if(bint) bint = inum > 0 && iden == 1;
			if(inum > 0) idm1(CHILD(index2), bfrac, bint);
			if(iden > 0) idm1(CHILD(index1)[0], bfrac, bint);
			bool b = false;
			if(!dofrac) bfrac = false;
			if(bint || bfrac) {
				Number nr(1, 1);
				if(inum > 0) idm2(CHILD(index2), bfrac, bint, nr);
				if(iden > 0) idm2(CHILD(index1)[0], bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) nr.recip();

					bool b_fail = false;
					if(inum > 1 && !CHILD(index2).isNumber()) {
						int i = idm3_test(b_fail, *this, nr, !po.allow_factorization, po, top_parent);
						if(i >= 0 && !b_fail && iden > 0) idm3_test(b_fail, CHILD(index1)[0], nr, !po.allow_factorization, po, top_parent);
					} else {
						if(inum != 0) idm3_test(b_fail, CHILD(index2), nr, !po.allow_factorization, po, top_parent);
						if(!b_fail && iden > 0) idm3_test(b_fail, CHILD(index1)[0], nr, !po.allow_factorization, po, top_parent);
					}
					if(!b_fail) {
						if(inum == 0) {
							PREPEND(MathStructure(nr));
							index1 += 1;
						} else if(inum > 1 && !CHILD(index2).isNumber()) {
							int i = idm3(*this, nr, !po.allow_factorization);
							if(i == 1) index1++;
							else if(i < 0) iden = 0;
						} else {
							idm3(CHILD(index2), nr, !po.allow_factorization);
						}
						if(iden > 0) {
							idm3(CHILD(index1)[0], nr, !po.allow_factorization);
						} else {
							MathStructure mstruct(nr);
							mstruct.raise(m_minus_one);
							insertChild(mstruct, index1);
						}
						b = true;
					}
				}
			}
			if(!b && po.show_ending_zeroes && po.restrict_to_parent_precision && top_parent->isApproximate() && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && inum > 0 && iden > 0) {
				bint = false;
				bool bint2 = false;
				idm1b(CHILD(index2), bint, bint2);
				if(bint) idm1b(CHILD(index1)[0], bint, bint2);
				if(bint && bint2) {
					Number nr;
					idm2b(CHILD(index2), nr);
					idm2b(CHILD(index1)[0], nr);
					idm3b(CHILD(index2), nr);
					idm3b(CHILD(index1)[0], nr);
				}
			}
			return b;
		}
		case STRUCT_DIVISION: {
			bool bint = true, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			idm1(CHILD(1), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				idm2(CHILD(1), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) nr.recip();

					bool b_fail = false;
					idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;
					idm3_test(b_fail, CHILD(1), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;

					idm3(CHILD(0), nr, !po.allow_factorization);
					idm3(CHILD(1), nr, !po.allow_factorization);
					return true;
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			bool bint = false, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					bool b_fail = false;
					idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;

					setToChild(1, true);
					idm3(*this, nr, !po.allow_factorization);
					transform_nocopy(STRUCT_DIVISION, new MathStructure(nr));
					SWAP_CHILDREN(0, 1);
					return true;
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(1).isMinusOne()) {
				bool bint = false, bfrac = false;
				idm1(CHILD(0), bfrac, bint);
				if(bint || bfrac) {
					Number nr(1, 1);
					idm2(CHILD(0), bfrac, bint, nr);
					if((bint || bfrac) && !nr.isOne()) {
						bool b_fail = false;
						idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
						if(b_fail) return false;

						idm3(CHILD(0), nr, !po.allow_factorization);
						transform(STRUCT_MULTIPLICATION);
						PREPEND(MathStructure(nr));
						return true;
					}
				}
				break;
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).improve_division_multipliers(po, top_parent)) b = true;
			}
			return b;
		}
	}
	return false;
}

bool use_prefix_with_unit(Unit *u, const PrintOptions &po) {
	if(!po.prefix && !po.use_unit_prefixes) {return u->referenceName() == "g" || u->referenceName() == "a";}
	if(po.prefix) return true;
	if(u->isCurrency()) return po.use_prefixes_for_currencies;
	if(po.use_prefixes_for_all_units) return true;
	return u->useWithPrefixesByDefault();
}
bool use_prefix_with_unit(const MathStructure &mstruct, const PrintOptions &po) {
	if(mstruct.isUnit()) return use_prefix_with_unit(mstruct.unit(), po);
	if(mstruct.isUnit_exp()) return use_prefix_with_unit(mstruct[0].unit(), po);
	return false;
}

bool has_prefix(const MathStructure &mstruct) {
	if(mstruct.isUnit()) return mstruct.prefix() != NULL;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(has_prefix(mstruct[i])) return true;
	}
	return false;
}

void MathStructure::setPrefixes(const PrintOptions &po, MathStructure *parent, size_t pindex) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			size_t i = SIZE, im = 0;
			bool b_im = false;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isUnit_exp()) {
					if(CHILD(i2).unit_exp_prefix()) {
						b = false;
						return;
					}
					if(!b) {
						if(use_prefix_with_unit(CHILD(i2), po)) {
							b = true;
							if(i > i2) {i = i2; b_im = false;}
						} else if(i < i2) {
							i = i2;
							b_im = false;
						}
					}
				} else if(CHILD(i2).isPower() && CHILD(i2)[0].isMultiplication()) {
					for(size_t i3 = 0; i3 < CHILD(i2)[0].size(); i3++) {
						if(CHILD(i2)[0][i3].isUnit_exp()) {
							if(CHILD(i2)[0][i3].unit_exp_prefix()) {
								b = false;
								return;
							}
							if(!b) {
								if(use_prefix_with_unit(CHILD(i2)[0][i3], po)) {
									b = true;
									if(i > i2) {
										i = i2;
										im = i3;
										b_im = true;
									}
									break;
								} else if(i < i2 || (i == i2 && im < i3)) {
									i = i2;
									im = i3;
									b_im = true;
								}
							}
						}
					}
				}
			}
			if(b) {
				Number exp(1, 1);
				Number exp2(1, 1);
				bool b2 = false;
				MathStructure *munit = NULL, *munit2 = NULL;
				if(b_im) munit = &CHILD(i)[0][im];
				else munit = &CHILD(i);
				if(CHILD(i).isPower()) {
					if(CHILD(i)[1].isNumber() && CHILD(i)[1].number().isInteger() && !CHILD(i)[1].number().isZero()) {
						if(b_im && munit->isPower()) {
							if((*munit)[1].isNumber() && (*munit)[1].number().isInteger() && !(*munit)[1].number().isZero()) {
								exp = CHILD(i)[1].number();
								exp *= (*munit)[1].number();
							} else {
								b = false;
							}
						} else {
							exp = CHILD(i)[1].number();
						}
					} else {
						b = false;
					}
				}
				if(po.use_denominator_prefix && !exp.isNegative()) {
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						if(CALCULATOR->aborted()) break;
						if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isNegative()) {
							if(CHILD(i2)[0].isUnit() && use_prefix_with_unit(CHILD(i2)[0], po)) {
								munit2 = &CHILD(i2)[0];
								if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
									break;
								}
								if(!b) {
									b = true;
									exp = CHILD(i2)[1].number();
									munit = munit2;
								} else {
									b2 = true;
									exp2 = CHILD(i2)[1].number();
								}
								break;
							} else if(CHILD(i2)[0].isMultiplication()) {
								bool b_break = false;
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && use_prefix_with_unit(CHILD(i2)[0][im2], po) && (CHILD(i2)[0][im2].isUnit() || (CHILD(i2)[0][im2][1].isNumber() && (CHILD(i2)[0][im2][1].number().isPositive() || (!b && CHILD(i2)[0][im2][1].number().isNegative())) && CHILD(i2)[0][im2][1].number().isInteger()))) {
										Number exp_multi(1);
										if(CHILD(i2)[0][im2].isUnit()) {
											munit2 = &CHILD(i2)[0][im2];
										} else {
											munit2 = &CHILD(i2)[0][im2][0];
											exp_multi = CHILD(i2)[0][im2][1].number();
										}
										b_break = true;
										if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
											break;
										}
										if(!b) {
											b = true;
											exp = CHILD(i2)[1].number();
											exp *= exp_multi;
											i = i2;
										} else {
											b2 = true;
											exp2 = CHILD(i2)[1].number();
											exp2 *= exp_multi;
										}
										break;
									}
								}
								if(b_break) break;
							}
						}
					}
				} else if(exp.isNegative() && b) {
					bool had_unit = false;
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						if(CALCULATOR->aborted()) break;
						bool b3 = false;
						if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isPositive()) {
							if(CHILD(i2)[0].isUnit()) {
								if(!use_prefix_with_unit(CHILD(i2)[0], po)) {
									had_unit = true;
								} else {
									munit2 = &CHILD(i2);
									if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
										break;
									}
									b3 = true;
									exp2 = exp;
									exp = CHILD(i2)[1].number();
								}
							} else if(CHILD(i2)[0].isMultiplication()) {
								bool b_break = false;
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && (CHILD(i2)[0][im2].isUnit() || (CHILD(i2)[0][im2][1].isNumber() && CHILD(i2)[0][im2][1].number().isPositive() && CHILD(i2)[0][im2][1].number().isInteger()))) {
										if(!use_prefix_with_unit(CHILD(i2)[0][im2], po)) {
											had_unit = true;
										} else {
											Number exp_multi(1);
											if(CHILD(i2)[0][im2].isUnit()) {
												munit2 = &CHILD(i2)[0][im2];
											} else {
												munit2 = &CHILD(i2)[0][im2][0];
												exp_multi = CHILD(i2)[0][im2][1].number();
											}
											b_break = true;
											if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
												break;
											}
											exp2 = exp;
											exp = CHILD(i2)[1].number();
											exp *= exp_multi;
											b3 = true;
											break;
										}
									}
								}
								if(b_break) break;
							}
						} else if(CHILD(i2).isUnit()) {
							if(!use_prefix_with_unit(CHILD(i2), po)) {
								had_unit = true;
							} else {
								if(CHILD(i2).prefix()) break;
								exp2 = exp;
								exp.set(1, 1, 0);
								b3 = true;
								munit2 = &CHILD(i2);
							}
						}
						if(b3) {
							if(po.use_denominator_prefix) {
								b2 = true;
								MathStructure *munit3 = munit;
								munit = munit2;
								munit2 = munit3;
							} else {
								munit = munit2;
							}
							had_unit = false;
							break;
						}
					}
					if(had_unit && !po.use_denominator_prefix) b = false;
				}
				Number exp10;
				if(b) {
					if(po.prefix) {
						if(po.prefix != CALCULATOR->getDecimalNullPrefix() && po.prefix != CALCULATOR->getBinaryNullPrefix()) {
							if(munit->isUnit()) munit->setPrefix(po.prefix);
							else (*munit)[0].setPrefix(po.prefix);
							if(CHILD(0).isNumber()) {
								CHILD(0).number() /= po.prefix->value(exp);
							} else {
								PREPEND(po.prefix->value(exp));
								CHILD(0).number().recip();
							}
						}
					} else if(po.use_unit_prefixes && CHILD(0).isNumber() && exp.isInteger()) {
						exp10 = CHILD(0).number();
						exp10.abs();
						exp10.intervalToMidValue();
						if(exp10.isLessThanOrEqualTo(Number(1, 1, 1000)) && exp10.isGreaterThanOrEqualTo(Number(1, 1, -1000))) {
							bool use_binary_prefix = (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit->isUnit() && munit->unit()->baseUnit()->referenceName() == "bit") || (munit->isPower() && (*munit)[0].unit()->baseUnit()->referenceName() == "bit"))));
							exp10.log(use_binary_prefix ? 2 : 10);
							exp10.intervalToMidValue();
							exp10.floor();
							if(b2 && exp10.isPositive() && (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit2->isUnit() && munit2->unit()->baseUnit()->referenceName() == "bit") || (munit2->isPower() && (*munit2)[0].unit()->baseUnit()->referenceName() == "bit"))))) b2 = false;
							if(b2 && use_binary_prefix && CALCULATOR->usesBinaryPrefixes() == 1 && exp10.isNegative()) {
								exp10.clear();
							} else if(b2) {
								Number tmp_exp(exp10);
								tmp_exp.setNegative(false);
								Number e1(use_binary_prefix ? 10 : 3, 1, 0);
								e1 *= exp;
								Number e2(use_binary_prefix ? 10 : 3, 1, 0);
								e2 *= exp2;
								e2.setNegative(false);
								int i4 = 0;
								while(true) {
									tmp_exp -= e1;
									if(!tmp_exp.isPositive()) {
										break;
									}
									if(exp10.isNegative()) i4++;
									tmp_exp -= e2;
									if(tmp_exp.isNegative()) {
										break;
									}
									if(!exp10.isNegative())	i4++;
								}
								e2.setNegative(exp10.isNegative());
								e2 *= i4;
								exp10 -= e2;
							}
							Prefix *p = (use_binary_prefix > 0 ? (Prefix*) CALCULATOR->getOptimalBinaryPrefix(exp10, exp) : (Prefix*) CALCULATOR->getOptimalDecimalPrefix(exp10, exp, po.use_all_prefixes));
							if(p) {
								Number test_exp(exp10);
								if(use_binary_prefix) test_exp -= ((BinaryPrefix*) p)->exponent(exp);
								else test_exp -= ((DecimalPrefix*) p)->exponent(exp);
								if(test_exp.isInteger()) {
									if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
										CHILD(0).number() /= p->value(exp);
										if(munit->isUnit()) munit->setPrefix(p);
										else (*munit)[0].setPrefix(p);
									}
								}
							}
						}
					} else if(!po.use_unit_prefixes) {
						Prefix *p = NULL;
						if((munit->isUnit() && munit->unit()->referenceName() == "g") || (munit->isPower() && (*munit)[0].unit()->referenceName() == "g")) {
							p = CALCULATOR->getExactDecimalPrefix(3);
						} else if((munit->isUnit() && munit->unit()->referenceName() == "a") || (munit->isPower() && (*munit)[0].unit()->referenceName() == "a")) {
							p = CALCULATOR->getExactDecimalPrefix(2);
						}
						if(p) {
							if(munit->isUnit()) munit->setPrefix(p);
							else (*munit)[0].setPrefix(p);
							if(CHILD(0).isNumber()) {
								CHILD(0).number() /= p->value(exp);
							} else {
								PREPEND(p->value(exp));
								CHILD(0).number().recip();
							}
						}
					}
					if(b2 && CHILD(0).isNumber() && (po.prefix || po.use_unit_prefixes) && (po.prefix != CALCULATOR->getDecimalNullPrefix() && po.prefix != CALCULATOR->getBinaryNullPrefix())) {
						exp10 = CHILD(0).number();
						exp10.abs();
						exp10.intervalToMidValue();
						if(exp10.isLessThanOrEqualTo(Number(1, 1, 1000)) && exp10.isGreaterThanOrEqualTo(Number(1, 1, -1000))) {
							bool use_binary_prefix = (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit2->isUnit() && munit2->unit()->baseUnit()->referenceName() == "bit") || (munit2->isPower() && (*munit2)[0].unit()->baseUnit()->referenceName() == "bit"))));
							exp10.log(use_binary_prefix ? 2 : 10);
							exp10.intervalToMidValue();
							exp10.floor();
							Prefix *p = (use_binary_prefix > 0 ? (Prefix*) CALCULATOR->getOptimalBinaryPrefix(exp10, exp2) : (Prefix*) CALCULATOR->getOptimalDecimalPrefix(exp10, exp2, po.use_all_prefixes));
							if(p) {
								Number test_exp(exp10);
								if(use_binary_prefix) test_exp -= ((BinaryPrefix*) p)->exponent(exp2);
								else test_exp -= ((DecimalPrefix*) p)->exponent(exp2);
								if(test_exp.isInteger()) {
									if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
										CHILD(0).number() /= p->value(exp2);
										if(munit2->isUnit()) munit2->setPrefix(p);
										else (*munit2)[0].setPrefix(p);
									}
								}
							}
						}
					}
				}
				return;
			}
			break;
		}
		case STRUCT_UNIT: {
			if(!o_prefix && (po.prefix && po.prefix != CALCULATOR->getDecimalNullPrefix() && po.prefix != CALCULATOR->getBinaryNullPrefix())) {
				transform(STRUCT_MULTIPLICATION, m_one);
				SWAP_CHILDREN(0, 1);
				setPrefixes(po, parent, pindex);
			}
			return;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				if(CHILD(1).isNumber() && CHILD(1).number().isReal() && !CHILD(0).prefix() && !o_prefix && (po.prefix && po.prefix != CALCULATOR->getDecimalNullPrefix() && po.prefix != CALCULATOR->getBinaryNullPrefix())) {
					transform(STRUCT_MULTIPLICATION, m_one);
					SWAP_CHILDREN(0, 1);
					setPrefixes(po, parent, pindex);
				}
				return;
			}
			break;
		}
		default: {}
	}
	if(po.prefix || !has_prefix(*this)) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).setPrefixes(po, this, i + 1);
		}
	}
}
bool split_unit_powers(MathStructure &mstruct);
bool split_unit_powers(MathStructure &mstruct) {
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		if(split_unit_powers(mstruct[i])) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		bool b2 = mstruct[1].isNumber();
		for(size_t i = 0; i < mstruct[0].size(); i++) {
			if(mstruct[0][i].isPower() && (!b2 || !mstruct[0][i][1].isNumber())) return b;
		}
		MathStructure mpower(mstruct[1]);
		mstruct.setToChild(1);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower()) mstruct[i][1].number() *= mpower.number();
			else mstruct[i].raise(mpower);
		}
		mstruct.childrenUpdated();
		return true;
	}
	return b;
}
void MathStructure::postFormatUnits(const PrintOptions &po, MathStructure *parent, size_t) {
	switch(m_type) {
		case STRUCT_DIVISION: {
			if(po.place_units_separately) {
				vector<size_t> nums;
				bool b1 = false, b2 = false;
				if(CHILD(0).isMultiplication()) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isUnit_exp()) {
							nums.push_back(i);
						} else {
							b1 = true;
						}
					}
					b1 = b1 && !nums.empty();
				} else if(CHILD(0).isUnit_exp()) {
					b1 = true;
				}
				vector<size_t> dens;
				if(CHILD(1).isMultiplication()) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isUnit_exp()) {
							dens.push_back(i);
						} else {
							b2 = true;
						}
					}
					b2 = b2 && !dens.empty();
				} else if(CHILD(1).isUnit_exp()) {
					if(CHILD(0).isUnit_exp()) {
						b1 = false;
					} else {
						b2 = true;
					}
				}
				if(b2 && !b1) b1 = true;
				if(b1) {
					MathStructure num = m_undefined;
					if(CHILD(0).isUnit_exp()) {
						num = CHILD(0);
						CHILD(0).set(m_one);
					} else if(nums.size() > 0) {
						num = CHILD(0)[nums[0]];
						for(size_t i = 1; i < nums.size(); i++) {
							num.multiply(CHILD(0)[nums[i]], i > 1);
						}
						for(size_t i = 0; i < nums.size(); i++) {
							CHILD(0).delChild(nums[i] + 1 - i);
						}
						if(CHILD(0).size() == 1) {
							CHILD(0).setToChild(1, true);
						}
					}
					MathStructure den = m_undefined;
					if(CHILD(1).isUnit_exp()) {
						den = CHILD(1);
						setToChild(1, true);
					} else if(dens.size() > 0) {
						den = CHILD(1)[dens[0]];
						for(size_t i = 1; i < dens.size(); i++) {
							den.multiply(CHILD(1)[dens[i]], i > 1);
						}
						for(size_t i = 0; i < dens.size(); i++) {
							CHILD(1).delChild(dens[i] + 1 - i);
						}
						if(CHILD(1).size() == 1) {
							CHILD(1).setToChild(1, true);
						}
					}
					if(num.isUndefined()) {
						transform(STRUCT_DIVISION, den);
					} else {
						if(!den.isUndefined()) {
							num.transform(STRUCT_DIVISION, den);
						}
						multiply(num, false);
					}
					if(CHILD(0).isDivision()) {
						if(CHILD(0)[0].isMultiplication()) {
							if(CHILD(0)[0].size() == 1) {
								CHILD(0)[0].setToChild(1, true);
							} else if(CHILD(0)[0].size() == 0) {
								CHILD(0)[0] = 1;
							}
						}
						if(CHILD(0)[1].isMultiplication()) {
							if(CHILD(0)[1].size() == 1) {
								CHILD(0)[1].setToChild(1, true);
							} else if(CHILD(0)[1].size() == 0) {
								CHILD(0).setToChild(1, true);
							}
						} else if(CHILD(0)[1].isOne()) {
							CHILD(0).setToChild(1, true);
						}
						if(CHILD(0).isDivision() && CHILD(0)[1].isNumber() && CHILD(0)[0].isMultiplication() && CHILD(0)[0].size() > 1 && CHILD(0)[0][0].isNumber()) {
							MathStructure *msave = new MathStructure;
							if(CHILD(0)[0].size() == 2) {
								msave->set(CHILD(0)[0][1]);
								CHILD(0)[0].setToChild(1, true);
							} else {
								msave->set(CHILD(0)[0]);
								CHILD(0)[0].setToChild(1, true);
								msave->delChild(1);
							}
							if(isMultiplication()) {
								insertChild_nocopy(msave, 2);
							} else {
								CHILD(0).multiply_nocopy(msave);
							}
						}
					}
					bool do_plural = po.short_multiplication;
					CHILD(0).postFormatUnits(po, this, 1);
					CHILD_UPDATED(0);
					switch(CHILD(0).type()) {
						case STRUCT_NUMBER: {
							if(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction()) {
								do_plural = false;
							}
							break;
						}
						case STRUCT_DIVISION: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[1].isNumber()) {
								if(CHILD(0)[0].number().isLessThanOrEqualTo(CHILD(0)[1].number())) {
									do_plural = false;
								}
							}
							break;
						}
						case STRUCT_INVERSE: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[0].number().isGreaterThanOrEqualTo(1)) {
								do_plural = false;
							}
							break;
						}
						default: {}
					}
					split_unit_powers(CHILD(1));
					switch(CHILD(1).type()) {
						case STRUCT_UNIT: {
							CHILD(1).setPlural(do_plural);
							break;
						}
						case STRUCT_POWER: {
							CHILD(1)[0].setPlural(do_plural);
							break;
						}
						case STRUCT_MULTIPLICATION: {
							if(po.limit_implicit_multiplication) CHILD(1)[0].setPlural(do_plural);
							else CHILD(1)[CHILD(1).size() - 1].setPlural(do_plural);
							break;
						}
						case STRUCT_DIVISION: {
							switch(CHILD(1)[0].type()) {
								case STRUCT_UNIT: {
									CHILD(1)[0].setPlural(do_plural);
									break;
								}
								case STRUCT_POWER: {
									CHILD(1)[0][0].setPlural(do_plural);
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(po.limit_implicit_multiplication) CHILD(1)[0][0].setPlural(do_plural);
									else CHILD(1)[0][CHILD(1)[0].size() - 1].setPlural(do_plural);
									break;
								}
								default: {}
							}
							break;
						}
						default: {}
					}
				}
			} else {
				for(size_t i = 0; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					CHILD(i).postFormatUnits(po, this, i + 1);
					CHILD_UPDATED(i);
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			b_plural = false;
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(SIZE > 1 && CHILD(1).isUnit_exp() && CHILD(0).isNumber()) {
				bool do_plural = po.short_multiplication && !(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction());
				size_t i = 2;
				for(; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
					} else {
						break;
					}
				}
				if(do_plural) {
					if(po.limit_implicit_multiplication) i = 1;
					else i--;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(true);
					} else {
						CHILD(i)[0].setPlural(true);
					}
				}
			} else if(SIZE > 0) {
				int last_unit = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(last_unit >= 0) {
						break;
					}
				}
				if(po.short_multiplication && last_unit > 0) {
					if(CHILD(last_unit).isUnit()) {
						CHILD(last_unit).setPlural(true);
					} else {
						CHILD(last_unit)[0].setPlural(true);
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				CHILD(0).setPlural(false);
				break;
			}
		}
		case STRUCT_NEGATE: {
			if(po.place_units_separately && (!parent || !parent->isAddition())) {
				CHILD(0).postFormatUnits(po, this, 1);
				CHILD_UPDATED(0);
				if(CHILD(0).isMultiplication() && SIZE > 0 && ((CHILD(0)[0].isDivision() && CHILD(0)[0][0].isInteger() && CHILD(0)[0][1].isInteger()) || (CHILD(0)[0].isInverse() && CHILD(0)[0][0].isInteger()))) {
					setToChild(1, true);
					CHILD(0).transform(STRUCT_NEGATE);
				}
				break;
			}
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				CHILD(i).postFormatUnits(po, this, i + 1);
				CHILD_UPDATED(i);
			}
		}
	}
}
void MathStructure::prefixCurrencies(const PrintOptions &po) {
	if(isMultiplication()) {
		int index = -1;
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(CHILD(i).isUnit_exp()) {
				if(CHILD(i).isUnit() && CHILD(i).unit()->isCurrency()) {
					const ExpressionName *ename = &CHILD(i).unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, CHILD(i).isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
					bool do_prefix = false;
					if(ename->reference) do_prefix = (hasNegativeSign() ? CALCULATOR->place_currency_code_before_negative : CALCULATOR->place_currency_code_before);
					else if(ename->abbreviation) do_prefix = (hasNegativeSign() ? CALCULATOR->place_currency_sign_before_negative : CALCULATOR->place_currency_sign_before);
					if(!do_prefix || index >= 0) {
						index = -1;
						break;
					}
					index = i;
				} else {
					index = -1;
					break;
				}
			}
		}
		if(index >= 0) {
			v_order.insert(v_order.begin(), v_order[index]);
			v_order.erase(v_order.begin() + (index + 1));
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).prefixCurrencies(po);
		}
	}
}
void remove_multi_one(MathStructure &mstruct) {
	if(mstruct.isMultiplication() && mstruct.size() > 1) {
		if(mstruct[0].isOne() && !mstruct[1].isUnit_exp() && (mstruct.size() != 2 || !mstruct[1].isFunction() || mstruct[1].function()->referenceName() != "cis" || mstruct[1].size() != 1)) {
			if(mstruct.size() == 2) mstruct.setToChild(2, true);
			else mstruct.delChild(1);
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		remove_multi_one(mstruct[i]);
	}
}
bool unnegate_multiplier(MathStructure &mstruct, const PrintOptions &po) {
	if(mstruct.isMultiplication() && mstruct.size() >= 2 && mstruct[0].isNumber() && mstruct[0].number().isNegative()) {
		for(size_t i = 1; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(mstruct[i].isAddition() || (mstruct[i].isPower() && mstruct[i][0].isAddition() && mstruct[i][1].isMinusOne())) {
				MathStructure *mden;
				if(mstruct[i].isAddition()) mden = &mstruct[i];
				else mden = &mstruct[i][0];
				bool b_pos = false, b_neg = false;
				for(size_t i2 = 0; i2 < mden->size(); i2++) {
					if((*mden)[i2].hasNegativeSign()) {
						b_neg = true;
					} else {
						b_pos = true;
					}
					if(b_neg && b_pos) break;
				}
				if(b_neg && b_pos) {
					for(size_t i2 = 0; i2 < mden->size(); i2++) {
						if((*mden)[i2].isNumber()) {
							(*mden)[i2].number().negate();
						} else if((*mden)[i2].isMultiplication() && (*mden)[i2].size() > 0) {
							if((*mden)[i2][0].isNumber()) {
								if((*mden)[i2][0].number().isMinusOne() && (*mden)[i2].size() > 1) {
									if((*mden)[i2].size() == 2) (*mden)[i2].setToChild(2, true);
									else (*mden)[i2].delChild(1);
								} else (*mden)[i2][0].number().negate();
							} else {
								(*mden)[i2].insertChild(m_minus_one, 1);
							}
						} else {
							(*mden)[i2].negate();
						}
					}
					mden->sort(po, false);
					if(mstruct[0].number().isMinusOne()) {
						if(mstruct.size() == 2) mstruct.setToChild(2, true);
						else mstruct.delChild(1);
					} else {
						mstruct[0].number().negate();
					}
					return true;
				}
			}
		}
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		if(unnegate_multiplier(mstruct[i], po)) {
			b = true;
		}
	}
	if(b) {
		mstruct.sort(po, false);
		return true;
	}
	return false;
}
Unit *default_angle_unit(const EvaluationOptions &eo) {
	switch(eo.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {return CALCULATOR->getDegUnit();}
		case ANGLE_UNIT_GRADIANS: {return CALCULATOR->getGraUnit();}
		case ANGLE_UNIT_RADIANS: {return CALCULATOR->getRadUnit();}
		default: {}
	}
	return NULL;
}

bool remove_angle_unit(MathStructure &m, Unit *u) {
	// remove angle unit from trigonometric function arguments
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(remove_angle_unit(m[i], u)) b_ret = true;
		if(m.isFunction() && m.function()->getArgumentDefinition(i + 1) && m.function()->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_ANGLE) {
			if(m[i].isUnit() && !m[i].prefix() && m[i].unit() == u) {
				m[i].set(1, 1, 0, true);
			} else if(m[i].isMultiplication()) {
				// f(a)*u: f(a)
				for(size_t i3 = 0; i3 < m[i].size(); i3++) {
					// ignore units with prefix
					if(m[i][i3].isUnit() && !m[i][i3].prefix() && m[i][i3].unit() == u) {
						m[i].delChild(i3 + 1, true);
						b_ret = true;
						break;
					}
				}
			} else if(m[i].isAddition()) {
				bool b = true;
				// f(a)*u+f(b)*u: f(a)+f(b)
				// check if unit is present in all terms first
				for(size_t i2 = 0; i2 < m[i].size(); i2++) {
					bool b2 = false;
					if(m[i][i2].isUnit() && !m[i][i2].prefix() && m[i][i2].unit() == u) {
						b2 = true;
					} else if(m[i][i2].isMultiplication()) {
						for(size_t i3 = 0; i3 < m[i][i2].size(); i3++) {
							if(m[i][i2][i3].isUnit() && !m[i][i2][i3].prefix() && m[i][i2][i3].unit() == u) {
								b2 = true;
								break;
							}
						}
					}
					if(!b2) {
						b = false;
						break;
					}
				}
				if(b) {
					b_ret = true;
					for(size_t i2 = 0; i2 < m[i].size(); i2++) {
						if(m[i][i2].isUnit() && !m[i][i2].prefix() && m[i][i2].unit() == u) {
							m[i][i2].set(1, 1, 0, true);
						} else {
							for(size_t i3 = 0; i3 < m[i][i2].size(); i3++) {
								if(m[i][i2][i3].isUnit() && !m[i][i2][i3].prefix() && m[i][i2][i3].unit() == u) {
									m[i][i2].delChild(i3 + 1, true);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	return b_ret;
}
bool MathStructure::removeDefaultAngleUnit(const EvaluationOptions &eo) {
	// remove default angle unit from trigonometric function arguments
	Unit *u = default_angle_unit(eo);
	if(!u) return false;
	return remove_angle_unit(*this, u);
}
void separate_units(MathStructure &m, MathStructure *parent = NULL, size_t index = 0) {
	if(m.isMultiplication() && parent && parent->isMultiplication() && m.containsType(STRUCT_UNIT, false, false, false)) {
		for(size_t i = 0; i < m.size();) {
			if(m[i].isUnit_exp()) {
				m[i].ref();
				parent->addChild_nocopy(&m[i]);
				m.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(m.size() == 0) parent->delChild(index);
		else if(m.size() == 1) m.setToChild(1, true);
	} else if(m.isPower() && m[1].isNumber() && m[1].number().isReal() && m[0].isMultiplication() && m[0].containsType(STRUCT_UNIT, false, false, false)) {
		MathStructure units;
		for(size_t i = 0; i < m[0].size();) {
			if(m[0][i].isUnit() || (m[0][i].isPower() && m[0][i][0].isUnit() && m[0][i][1].isNumber() && m[0][i][1].number().isReal())) {
				if(!m[0][i].isPower() || !m[0][i][1].number().multiply(m[1].number())) {
					m[0][i].raise(m[1]);
				}
				m[0][i].ref();
				units.addChild_nocopy(&m[0][i]);
				m[0].delChild(i + 1);
			} else {
				i++;
			}
		}
		if(units.size() > 0) {
			if((!parent || !parent->isMultiplication()) && m[0].size() == 0) {
				if(units.size() == 1) units.setToChild(1);
				else units.setType(STRUCT_MULTIPLICATION);
				m.set_nocopy(units, true);
			} else {
				if(parent && parent->isMultiplication() && m[0].size() == 0) parent->delChild(index);
				else if(m[0].size() == 1) m[0].setToChild(1, true);
				for(size_t i = 0; i < units.size(); i++) {
					units[i].ref();
					if(parent && parent->isMultiplication()) parent->addChild_nocopy(&units[i]);
					else m.multiply_nocopy(&units[i], true);
				}
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		separate_units(m[i], &m, i + 1);
	}
}
void MathStructure::format(const PrintOptions &po) {
	if(!po.preserve_format) {
		if(po.place_units_separately) {
			// a*u+b*u=(a+b)*u
			factorizeUnits();
			separate_units(*this);
		}
		sort(po);
		// 5000 u = 5 ku
		setPrefixes(po);
		// -1/(a-b)=1/(b-a)
		unnegate_multiplier(*this, po);
		// a(bx+y)=abx+ay if a and/or b are rational number displayed approximately
		fix_approximate_multiplier(*this, po);
		if(po.improve_division_multipliers) {
			// 0.5x/y=x/(2y)
			if(improve_division_multipliers(po)) sort(po);
		}
		// 1*a=a
		remove_multi_one(*this);
	}
	formatsub(po, NULL, 0, true, this);
	if(!po.preserve_format) {
		postFormatUnits(po);
		if(po.sort_options.prefix_currencies) {
			prefixCurrencies(po);
		}
	}
}

bool is_unit_multiadd(const MathStructure &m) {
	for(size_t i = 0; i < m.size(); i++) {
		if(!is_unit_multiexp(m[i]) && (!m[i].isMultiplication() || m[i].size() <= 1 || !m[i][0].isNumber() || !is_unit_multiexp(m[i][1]))) return false;
	}
	return true;
}

void MathStructure::formatsub(const PrintOptions &po, MathStructure *parent, size_t pindex, bool recursive, MathStructure *top_parent) {

	if(recursive) {
		size_t first_neg_exp = SIZE;
		if(m_type == STRUCT_MULTIPLICATION && po.place_units_separately && !po.negative_exponents) {
			// use negative exponents (instead of division) for units if fist unit has negative exponent (this should ordinarily mean that all subsequent units also has negative exponents)
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isUnit_exp()) {
					if(!CHILD(i).isPower() || !CHILD(i)[1].hasNegativeSign()) break;
					first_neg_exp = i;
					break;
				}
			}
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(i >= first_neg_exp) {
				PrintOptions po2 = po;
				po2.negative_exponents = true;
				CHILD(i).formatsub(po2, this, i + 1, true, top_parent);
			} else if(po.number_fraction_format == FRACTION_COMBINED && (m_type == STRUCT_FUNCTION || m_type == STRUCT_POWER || (m_type == STRUCT_ADDITION && (!po.place_units_separately || !is_unit_multiadd(*this))) || (m_type == STRUCT_MULTIPLICATION && SIZE > 1 && (!po.place_units_separately || !CHILD(0).isNumber() || !is_unit_multiexp(CHILD(1)))))) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				CHILD(i).formatsub(po2, this, i + 1, true, top_parent);
			} else if(!po.preserve_format && i == 1 && m_type == STRUCT_POWER && po.number_fraction_format < FRACTION_FRACTIONAL && CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().isInteger() && CHILD(1).number().numeratorIsLessThan(10) && CHILD(1).number().numeratorIsGreaterThan(-10) && CHILD(1).number().denominatorIsLessThan(10)) {
				// always display rational number exponents with small numerator and denominator as fraction (e.g. 5^(2/3) instead of 5^0.666...)
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				CHILD(i).formatsub(po2, this, i + 1, false, top_parent);
			} else {
				CHILD(i).formatsub(po, this, i + 1, true, top_parent);
			}
			CHILD_UPDATED(i);
		}
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			break;
		}
		case STRUCT_NEGATE: {
			break;
		}
		case STRUCT_DIVISION: {
			if(po.preserve_format) break;
			if(CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				// (-a-b)/(c-d)=-(a+b)/(c-d); (-a+b)/(-c-d)=(a-b)/(c+d)
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				bool b = CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate();
				if(b) {
					imin++;
					for(size_t i = 1; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							imin++;
						} else {
							imin--;
						}
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					if(b) {
						for(size_t i = 0; i < CHILD(1).size(); i++) {
							if(CHILD(1)[i].isNegate()) {
								CHILD(1)[i].setToChild(1, true);
							} else {
								CHILD(1)[i].transform(STRUCT_NEGATE);
							}
						}
					} else {
						transform(STRUCT_NEGATE);
					}
					break;
				}
			} else if(CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate()) {
				// (a+b)/(-c-d)=-(a+b)/(c+d)
				int imin = 1;
				for(size_t i = 1; i < CHILD(1).size(); i++) {
					if(CHILD(1)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							CHILD(1)[i].setToChild(1, true);
						} else {
							CHILD(1)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			if(po.preserve_format) break;
			if((!parent || !parent->isMultiplication()) && CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				// (-a-b+c)^-1=-(a+b-c)^-1
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(po.preserve_format) break;

			if(CHILD(0).isNegate()) {
				if(CHILD(0)[0].isOne()) {
					// (-1)x=-(x)
					ERASE(0);
					if(SIZE == 1) setToChild(1, true);
				} else {
					// -(a)x=-(ax)
					CHILD(0).setToChild(1, true);
				}
				formatsub(po, parent, pindex, false, top_parent);
				if((!parent || !parent->isAddition()) && (isMultiplication() && SIZE > 0 && ((CHILD(0).isDivision() && CHILD(0)[0].isInteger() && CHILD(0)[1].isInteger()) || (CHILD(0).isInverse() && CHILD(0)[0].isInteger())))) {
					CHILD(0).transform(STRUCT_NEGATE);
				} else {
					transform(STRUCT_NEGATE);
				}
				break;
			}

			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).isInverse()) {
					b = true;
					break;
				} else if(CHILD(i).isDivision()) {
					if(!CHILD(i)[0].isNumber() || !CHILD(i)[1].isNumber() || CHILD(i)[0].number().isOne()) {
						b = true;
						break;
					}
				}
			}

			if(b) {
				MathStructure *den = new MathStructure();
				MathStructure *num = new MathStructure();
				num->setUndefined();
				short ds = 0, ns = 0;
				MathStructure *mnum = NULL, *mden = NULL;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isInverse()) {
						mden = &CHILD(i)[0];
					} else if(CHILD(i).isDivision()) {
						mnum = &CHILD(i)[0];
						mden = &CHILD(i)[1];
					} else {
						mnum = &CHILD(i);
					}
					if(mnum && !mnum->isOne()) {
						if(ns > 0) {
							if(mnum->isMultiplication() && num->isNumber()) {
								for(size_t i2 = 0; i2 < mnum->size(); i2++) {
									num->multiply((*mnum)[i2], true);
								}
							} else {
								num->multiply(*mnum, ns > 1);
							}
						} else {
							num->set(*mnum);
						}
						ns++;
						mnum = NULL;
					}
					if(mden) {
						if(ds > 0) {
							if(mden->isMultiplication() && den->isNumber()) {
								for(size_t i2 = 0; i2 < mden->size(); i2++) {
									den->multiply((*mden)[i2], true);
								}
							} else {
								den->multiply(*mden, ds > 1);
							}
						} else {
							den->set(*mden);
						}
						ds++;
						mden = NULL;
					}
				}
				clear(true);
				m_type = STRUCT_DIVISION;
				if(num->isUndefined()) num->set(m_one);
				APPEND_POINTER(num);
				APPEND_POINTER(den);
				num->formatsub(po, this, 1, false, top_parent);
				den->formatsub(po, this, 2, false, top_parent);
				formatsub(po, parent, pindex, false, top_parent);
				break;
			}

			size_t index = 0;
			if(CHILD(0).isOne()) {
				index = 1;
			}
			switch(CHILD(index).type()) {
				case STRUCT_POWER: {
					if(!CHILD(index)[0].isUnit_exp()) {
						break;
					}
				}
				case STRUCT_UNIT: {
					if(index == 0) {
						if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && (!parent->isDivision() || pindex != 2))) {
							PREPEND(m_one);
							break;
						}
					}
					break;
				}
				case STRUCT_FUNCTION: {
					if(index == 1 && SIZE == 2 && CHILD(index).function()->referenceName() == "cis" && CHILD(index).size() == 1) break;
				}
				default: {
					if(index == 1) {
						ERASE(0);
						if(SIZE == 1) {
							setToChild(1, true);
						}
						break;
					}
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			if(po.preserve_format) break;
			if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2))) {
				// u = 1 u
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_POWER: {
			if(po.preserve_format) break;
			if(CHILD(1).isNegate() && ((!po.negative_exponents && parent != NULL) || !CHILD(0).isUnit()) && (!CHILD(0).isVector() || !CHILD(1).isMinusOne())) {
				if(CHILD(1)[0].isOne()) {
					// f(a)^-1=1/f(a)
					m_type = STRUCT_INVERSE;
					ERASE(1);
				} else {
					// f(a)^-b=1/f(a)^b
					CHILD(1).setToChild(1, true);
					transform(STRUCT_INVERSE);
				}
				formatsub(po, parent, pindex, true, top_parent);
				break;
			} else if(po.halfexp_to_sqrt && ((CHILD(1).isDivision() && CHILD(1)[0].isNumber() && CHILD(1)[0].number().isInteger() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && (((!po.negative_exponents || !CHILD(0).isUnit()) && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1)[0].isOne())) || (CHILD(1).isNumber() && CHILD(1).number().denominatorIsTwo() && (((!po.negative_exponents || !CHILD(0).isUnit()) && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1).number().numeratorIsOne())) || (CHILD(1).isInverse() && CHILD(1)[0].isNumber() && CHILD(1)[0].number() == 2))) {
				if(CHILD(1).isInverse() || (CHILD(1).isDivision() && CHILD(1)[0].number().isOne()) || (CHILD(1).isNumber() && CHILD(1).number().numeratorIsOne())) {
					// f(a)^(1/2)=sqrt(f(a))
					m_type = STRUCT_FUNCTION;
					ERASE(1)
					setFunctionId(FUNCTION_ID_SQRT);
				} else {
					// f(a)^(b+1/2)=f(b)^(sqrt(f(a))
					if(CHILD(1).isNumber()) {
						// f(a)^b
						CHILD(1).number() -= nr_half;
					} else {
						// f(a)^(b/2)
						Number nr = CHILD(1)[0].number();
						nr /= CHILD(1)[1].number();
						nr.floor();
						CHILD(1).set(nr);
					}
					if(CHILD(1).number().isOne()) {
						setToChild(1, true);
						if(parent && parent->isMultiplication()) {
							parent->insertChild_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), this, NULL), pindex + 1);
						} else {
							multiply_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), this, NULL));
						}
					} else {
						if(parent && parent->isMultiplication()) {
							parent->insertChild_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), &CHILD(0), NULL), pindex + 1);
						} else {
							multiply_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), &CHILD(0), NULL));
						}
					}
				}
				formatsub(po, parent, pindex, false, top_parent);
				break;
			} else if(po.exp_to_root && CHILD(0).representsNonNegative(true) && ((CHILD(1).isDivision() && CHILD(1)[0].isNumber() && CHILD(1)[0].number().isInteger() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isGreaterThan(1) && CHILD(1)[1].number().isLessThan(10) && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1)[0].isOne())) || (CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().isInteger() && CHILD(1).number().denominatorIsLessThan(10) && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1).number().numeratorIsOne())) || (CHILD(1).isInverse() && CHILD(1)[0].isNumber()  && CHILD(1)[0].number().isInteger() && CHILD(1)[0].number().isPositive() && CHILD(1)[0].number().isLessThan(10)))) {
				// f(a)^(b/c)=root(f(a),c)^b
				Number nr_int, nr_num, nr_den;
				if(CHILD(1).isNumber()) {
					nr_num = CHILD(1).number().numerator();
					nr_den = CHILD(1).number().denominator();
				} else if(CHILD(1).isDivision()) {
					nr_num.set(CHILD(1)[0].number());
					nr_den.set(CHILD(1)[1].number());
				} else if(CHILD(1).isInverse()) {
					nr_num.set(1, 1, 0);
					nr_den.set(CHILD(1)[0].number());
				}
				if(!nr_num.isOne() && (nr_num - 1).isIntegerDivisible(nr_den)) {
					nr_int = nr_num;
					nr_int--;
					nr_int.divide(nr_den);
					nr_num = 1;
				}
				MathStructure mbase(CHILD(0));
				CHILD(1) = nr_den;
				m_type = STRUCT_FUNCTION;
				setFunctionId(FUNCTION_ID_ROOT);
				formatsub(po, parent, pindex, false, top_parent);
				if(!nr_num.isOne()) {
					raise(nr_num);
					formatsub(po, parent, pindex, false, top_parent);
				}
				if(!nr_int.isZero()) {
					if(!nr_int.isOne()) mbase.raise(nr_int);
					multiply(mbase);
					sort(po);
					formatsub(po, parent, pindex, false, top_parent);
				}
				break;
			}
			if(CHILD(0).isUnit_exp() && (!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2)))) {
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(po.preserve_format) break;
			if(o_function->id() == FUNCTION_ID_ROOT && SIZE == 2 && CHILD(1) == 3) {
				// root(f(a),3)=cbrt(f(a))
				ERASE(1)
				setFunctionId(FUNCTION_ID_CBRT);
			} else if(o_function->id() == FUNCTION_ID_INTERVAL && SIZE == 2 && CHILD(0).isAddition() && CHILD(0).size() == 2 && CHILD(1).isAddition() && CHILD(1).size() == 2) {
				// interval(f(a)+c,f(a)-c)=uncertainty(f(a),c)
				MathStructure *mmid = NULL, *munc = NULL;
				if(CHILD(0)[0].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[1];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				} else if(CHILD(0)[0].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				}
				if(mmid && munc) {
					setFunctionId(FUNCTION_ID_UNCERTAINTY);
					mmid->ref();
					munc->ref();
					CLEAR
					APPEND_POINTER(mmid)
					APPEND_POINTER(munc)
					APPEND(m_zero)
				}
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(o_variable == CALCULATOR->getVariableById(VARIABLE_ID_PLUS_INFINITY) || o_variable == CALCULATOR->getVariableById(VARIABLE_ID_MINUS_INFINITY)) {
				// replace infinity variable with infinity number
				set(((KnownVariable*) o_variable)->get());
			}
			break;
		}
		case STRUCT_NUMBER: {
			bool force_fraction = false;
			if(!po.preserve_format && parent && parent->isMultiplication() && o_number.isRational()) {
				// always show fraction format for rational number a in a(f(b)+f(c))
				for(size_t i = 0; i < parent->size(); i++) {
					if((*parent)[i].isAddition()) {
						force_fraction = true;
						break;
					}
				}
			}

			if((o_number.isNegative() || ((parent || po.interval_display != INTERVAL_DISPLAY_SIGNIFICANT_DIGITS) && o_number.isInterval() && o_number.isNonPositive())) && (po.base != BASE_CUSTOM || !CALCULATOR->customOutputBase().isNegative())) {
				if((((po.base != 2 || !po.twos_complement) && (po.base != 16 || !po.hexadecimal_twos_complement)) || !o_number.isInteger()) && (!o_number.isMinusInfinity() || (parent && parent->isAddition()))) {
					// a=-(-a), if a is a negative number (or a is interval from negative value to 0), and not using two's complement and not using negative number base
					o_number.negate();
					transform(STRUCT_NEGATE);
					formatsub(po, parent, pindex, true, top_parent);
				}
			} else if((force_fraction || po.number_fraction_format >= FRACTION_FRACTIONAL || po.base == BASE_ROMAN_NUMERALS || po.number_fraction_format == FRACTION_DECIMAL_EXACT) && po.base > BASE_FP16 && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger() && (force_fraction || !o_number.isApproximate())) {
				// split rational number in numerator and denominator, if display of fractions is requested for rational numbers and number base is not sexagesimal and number is not approximate

				InternalPrintStruct ips_n;

				// parent approximate status affects the display of numbers if force_fraction is not true
				if(!force_fraction && (isApproximate() || (top_parent && top_parent->isApproximate()))) ips_n.parent_approximate = true;

				// if current mode and parent precision dictates showing of ending zeroes, number is not shown as fraction
				if(po.show_ending_zeroes && po.restrict_to_parent_precision && ips_n.parent_approximate && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && (o_number.numeratorIsGreaterThan(9) || o_number.numeratorIsLessThan(-9) || o_number.denominatorIsGreaterThan(9))) {
					break;
				}
				ips_n.parent_precision = precision();
				if(top_parent && top_parent->precision() < 0 && top_parent->precision() < ips_n.parent_precision) ips_n.parent_precision = top_parent->precision();

				bool approximately_displayed = false;
				PrintOptions po2 = po;
				po2.is_approximate = &approximately_displayed;
				po2.indicate_infinite_series = false;
				if(force_fraction && (po2.number_fraction_format == FRACTION_DECIMAL || po2.number_fraction_format == FRACTION_DECIMAL_EXACT)) po2.number_fraction_format = FRACTION_FRACTIONAL;
				if(!force_fraction && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && po.number_fraction_format == FRACTION_DECIMAL_EXACT) {
					// if FRACTION_DECIMAL_EXACT is active, numbers is not displayed as fraction if they can be shown exact using decimals
					po2.number_fraction_format = FRACTION_DECIMAL;
					o_number.print(po2, ips_n);
					if(!approximately_displayed) break;
					approximately_displayed = false;
				}

				// test if numerator and denominator is displayed exact using current mode
				Number num(o_number.numerator());
				if(po.number_fraction_format == FRACTION_COMBINED) {
					num.mod(o_number.denominator());
				}
				Number den(o_number.denominator());
				if(isApproximate()) {
					num.setApproximate();
					den.setApproximate();
				}
				num.print(po2, ips_n);
				if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
					den.print(po2, ips_n);
					if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
						if(po.number_fraction_format == FRACTION_COMBINED && !o_number.isFraction()) {
							// mixed fraction format (e.g. 5/3=1+2/3)
							Number nr_int(o_number);
							nr_int.trunc();
							if(isApproximate()) nr_int.setApproximate();
							nr_int.print(po2, ips_n);
							if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
								set(nr_int);
								MathStructure *mterm;
								if(num.isOne()) {
									mterm = new MathStructure(den);
									mterm->transform(STRUCT_INVERSE);
								} else {
									mterm = new MathStructure(num);
									mterm->transform(STRUCT_DIVISION, den);
								}
								add_nocopy(mterm);
								break;
							} else {
								approximately_displayed = false;
								num = o_number.numerator();
								if(isApproximate()) num.setApproximate();
								num.print(po2, ips_n);
							}
						}
						if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
							// both numerator and denominator is displayed exact: split up number
							clear(true);
							if(num.isOne()) {
								m_type = STRUCT_INVERSE;
							} else {
								m_type = STRUCT_DIVISION;
								APPEND_NEW(num);
							}
							APPEND_NEW(den);
						}
					}
				}
			} else if(o_number.hasImaginaryPart()) {
				if(o_number.hasRealPart()) {
					// split up complex number in real and imaginary part (Z=re(Z)+im(Z)*i)
					Number re(o_number.realPart());
					Number im(o_number.imaginaryPart());
					MathStructure *mstruct = new MathStructure(im);
					if(im.isOne()) {
						mstruct->set(CALCULATOR->getVariableById(VARIABLE_ID_I));
					} else {
						mstruct->multiply_nocopy(new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_I)));
						if(CALCULATOR->getVariableById(VARIABLE_ID_I)->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name == "j") mstruct->swapChildren(1, 2);
					}
					o_number = re;
					add_nocopy(mstruct);
					formatsub(po, parent, pindex, true, top_parent);
				} else {
					// transform imaginary number to imaginary part * i (Z=im(Z)*i)
					Number im(o_number.imaginaryPart());
					if(im.isOne()) {
						set(CALCULATOR->getVariableById(VARIABLE_ID_I), true);
					} else if(im.isMinusOne()) {
						set(CALCULATOR->getVariableById(VARIABLE_ID_I), true);
						transform(STRUCT_NEGATE);
					} else {
						o_number = im;
						multiply_nocopy(new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_I)));
						if(CALCULATOR->getVariableById(VARIABLE_ID_I)->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name == "j") SWAP_CHILDREN(0, 1);
					}
					formatsub(po, parent, pindex, true, top_parent);
				}
			}
			break;
		}
		default: {}
	}
}

int namelen(const MathStructure &mstruct, const PrintOptions &po, const InternalPrintStruct&, bool *abbreviated = NULL) {
	// returns the length of the name used (for mstruct) with the current mode (and if the name is an abbreviation)
	const string *str;
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			const ExpressionName *ename = &mstruct.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_VARIABLE:  {
			const ExpressionName *ename = &mstruct.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC:  {
			str = &mstruct.symbol();
			if(abbreviated) *abbreviated = false;
			break;
		}
		case STRUCT_UNIT:  {
			const ExpressionName *ename = &mstruct.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		default: {if(abbreviated) *abbreviated = false; return 0;}
	}
	if(text_length_is_one(*str)) return 1;
	return str->length();
}

bool MathStructure::needsParenthesis(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool flat_division, bool) const {
	// determines, using the type of the parent, the type of the child, the index of the child, and if division is displayed on one line or not (only relevant in the GUI), if this child should be displayed surrounded by parentheses
	switch(parent.type()) {
		case STRUCT_MULTIPLICATION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis || (index > 1 && SIZE > 0 && !is_unit_multiexp(*this));}
				case STRUCT_DIVISION: {return flat_division && (index < parent.size() || po.excessive_parenthesis);}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis || index > 1 || CHILD(0).needsParenthesis(po, ips, parent, index, flat_division);}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return o_function->id() == FUNCTION_ID_UNCERTAINTY;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || (o_number.hasImaginaryPart() && o_number.hasRealPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_POWER: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_FUNCTION: {return o_function->id() == FUNCTION_ID_UNCERTAINTY;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return (flat_division || po.excessive_parenthesis) && (o_number.isInfinite() || o_number.hasImaginaryPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_ADDITION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return index > 1 || po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_POWER: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return true;}
				case STRUCT_NEGATE: {return index == 1 || CHILD(0).needsParenthesis(po, ips, parent, index, flat_division);}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return o_function->id() == FUNCTION_ID_UNCERTAINTY;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || o_number.hasImaginaryPart();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_NEGATE: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return true;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || (o_number.hasImaginaryPart() && o_number.hasRealPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return false;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_COMPARISON: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return po.excessive_parenthesis;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_BITWISE_NOT: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return true;}
				case STRUCT_INVERSE: {return true;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return true;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return true;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return po.excessive_parenthesis;}
				case STRUCT_VECTOR: {return po.excessive_parenthesis;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis;}
				case STRUCT_VARIABLE: {return po.excessive_parenthesis;}
				case STRUCT_ABORTED: {return po.excessive_parenthesis;}
				case STRUCT_SYMBOLIC: {return po.excessive_parenthesis;}
				case STRUCT_UNIT: {return po.excessive_parenthesis;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				default: {return true;}
			}
		}
		case STRUCT_FUNCTION: {
			return false;
		}
		case STRUCT_VECTOR: {
			return false;
		}
		default: {
			return true;
		}
	}
}

int MathStructure::neededMultiplicationSign(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool par, bool par_prev, bool flat_division, bool flat_power) const {
	// returns the suggested multiplication sign in front of this MathStrcture

	// do not display anything on front of the first factor (this function is normally not called in this case)
	if(index <= 1) return MULTIPLICATION_SIGN_NONE;
	// short multiplication is disabled or number base might use digits other than 0-9, alawys show multiplication symbol
	if(!po.short_multiplication || po.base > 10 || po.base < 2) return MULTIPLICATION_SIGN_OPERATOR;
	// no multiplication sign between factors in parentheses
	if(par_prev && par) return MULTIPLICATION_SIGN_NONE;
	if(par_prev) {
		// (a)*u=(a) u
		if(is_unit_multiexp(*this)) return MULTIPLICATION_SIGN_SPACE;
		if(isUnknown_exp()) {
			// (a)*"xy"=(a) "xy", (a)*"xy"^b=(a) "xy"^b, (a)*x=(a)x, (a)*x^b=ax^b
			return (namelen(isPower() ? CHILD(0) : *this, po, ips, NULL) > 1 ? MULTIPLICATION_SIGN_SPACE : MULTIPLICATION_SIGN_NONE);
		}
		if(isMultiplication() && SIZE > 0) {
			// (a)*uv=(a) uv
			if(CHILD(0).isUnit_exp()) return MULTIPLICATION_SIGN_SPACE;
			if(CHILD(0).isUnknown_exp()) {
				// (a)*"xy"z=(a) "xy"z, (a)*xy=(a)xy
				return (namelen(CHILD(0).isPower() ? CHILD(0)[0] : CHILD(0), po, ips, NULL) > 1 ? MULTIPLICATION_SIGN_SPACE : MULTIPLICATION_SIGN_NONE);
			}
		} else if(isDivision()) {
			// (a)*(u1/u2)=(a) u1/u2
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).isUnit_exp()) {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			}
			return MULTIPLICATION_SIGN_SPACE;
		}
		// (a)*bc
		return MULTIPLICATION_SIGN_OPERATOR;
	}
	// type of factor in front in this factor
	int t = parent[index - 2].type();
	// a^b*c (if b is not shown using superscript or similar)
	if(flat_power && t == STRUCT_POWER) {
		if(!po.place_units_separately || !parent[index - 2].isUnit_exp()) return MULTIPLICATION_SIGN_OPERATOR;
	}
	// a^b*(c)=a^b (c)
	if(par && t == STRUCT_POWER) return MULTIPLICATION_SIGN_SPACE;
	// a*(b)=a(b)
	if(par) return MULTIPLICATION_SIGN_NONE;
	// check if involved names have only one character
	bool abbr_prev = false, abbr_this = false;
	int namelen_this = namelen(*this, po, ips, &abbr_this);
	int namelen_prev = namelen(parent[index - 2], po, ips, &abbr_prev);
	switch(t) {
		case STRUCT_MULTIPLICATION: {break;}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {if(flat_division) return MULTIPLICATION_SIGN_OPERATOR; return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {
			if(m_type == STRUCT_UNIT && parent[index - 2][0].isUnit()) {
				namelen(parent[index - 2], po, ips, &abbr_prev);
				if(po.place_units_separately) return MULTIPLICATION_SIGN_OPERATOR_SHORT;
				else if(!flat_power && !po.limit_implicit_multiplication && !abbr_prev && !abbr_this) return MULTIPLICATION_SIGN_SPACE;
				else return MULTIPLICATION_SIGN_OPERATOR;
			}
			break;
		}
		case STRUCT_NEGATE: {break;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {break;}
		case STRUCT_NUMBER: {break;}
		case STRUCT_VARIABLE: {break;}
		case STRUCT_ABORTED: {break;}
		case STRUCT_SYMBOLIC: {break;}
		case STRUCT_UNIT: {
			if(m_type == STRUCT_UNIT) {
				if(po.place_units_separately) {
					return MULTIPLICATION_SIGN_OPERATOR_SHORT;
				} else if(!po.limit_implicit_multiplication && !abbr_prev && !abbr_this) {
					return MULTIPLICATION_SIGN_SPACE;
				} else {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			} else if(m_type == STRUCT_NUMBER) {
				if(namelen_prev > 1) {
					return MULTIPLICATION_SIGN_SPACE;
				} else {
					return MULTIPLICATION_SIGN_NONE;
				}
			}
			//return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {break;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {if(SIZE > 0) {return CHILD(0).neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);} return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_INVERSE: {if(flat_division) {return m_one.neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);} return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_DIVISION: {if(flat_division) {return CHILD(0).neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);} return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {return CHILD(0).neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);}
		case STRUCT_NEGATE: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_NUMBER: {
			if(t == STRUCT_VARIABLE && parent[index - 2].variable() == CALCULATOR->getVariableById(VARIABLE_ID_I)) return MULTIPLICATION_SIGN_NONE;
			return MULTIPLICATION_SIGN_OPERATOR;
		}
		case STRUCT_VARIABLE: {
			if(!o_variable->isRegistered() && o_variable->getName(1).name.length() > 0 && o_variable->getName(1).name[0] >= '0' && o_variable->getName(1).name[0] <= '9') return MULTIPLICATION_SIGN_OPERATOR;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			if(po.limit_implicit_multiplication && t != STRUCT_NUMBER) return MULTIPLICATION_SIGN_OPERATOR;
			if(t != STRUCT_NUMBER && ((namelen_prev > 1 || namelen_this > 1) || equals(parent[index - 2]))) return MULTIPLICATION_SIGN_OPERATOR;
			if(namelen_this > 1) return MULTIPLICATION_SIGN_SPACE;
			return MULTIPLICATION_SIGN_NONE;
		}
		case STRUCT_UNIT: {
			if(o_unit == CALCULATOR->getDegUnit() && print(po) == SIGN_DEGREE) {
				return MULTIPLICATION_SIGN_NONE;
			}
			return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {return MULTIPLICATION_SIGN_OPERATOR;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
}

ostream& operator << (ostream &os, const MathStructure &mstruct) {
	os << format_and_print(mstruct);
	return os;
}
string MathStructure::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	return print(po, false, 0, TAG_TYPE_HTML, ips);
}
string MathStructure::print(const PrintOptions &po, bool format, int colorize, int tagtype, const InternalPrintStruct &ips) const {
	if(ips.depth == 0 && po.is_approximate) *po.is_approximate = false;
	string print_str;
	InternalPrintStruct ips_n = ips;
	if(isApproximate()) ips_n.parent_approximate = true;
	if(precision() >= 0 && (ips_n.parent_precision < 0 || precision() < ips_n.parent_precision)) ips_n.parent_precision = precision();
	switch(m_type) {
		case STRUCT_NUMBER: {
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str = (colorize == 2 ? "\033[0;96m" : "\033[0;36m");
			print_str += o_number.print(po, ips_n);
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			if(po.allow_non_usable) {
				print_str = s_sym;
			} else {
				if((text_length_is_one(s_sym) && s_sym.find("\'") == string::npos) || s_sym.find("\"") != string::npos) {
					print_str = "\'";
					print_str += s_sym;
					print_str += "\'";
				} else {
					print_str = "\"";
					print_str += s_sym;
					print_str += "\"";
				}
			}
			if((format && m_type != STRUCT_ABORTED) || (colorize && tagtype == TAG_TYPE_TERMINAL)) {
				if(tagtype == TAG_TYPE_TERMINAL) {
					if(format) print_str.insert(0, "\033[3m");
					if(colorize && m_type == STRUCT_ABORTED) print_str.insert(0, colorize == 2 ? "\033[0;91m" : "\033[0;31m");
					else if(colorize) print_str.insert(0, colorize == 2 ? "\033[0;93m" : "\033[0;33m");
					if(format) print_str += "\033[23m";
					if(colorize) print_str += "\033[0m";
				} else {
					print_str.insert(0, "<i>");
					print_str += "</i>";
				}
			}
			break;
		}
		case STRUCT_DATETIME: {
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str = (colorize == 2 ? "\033[0;96m" : "\033[0;36m");
			print_str += "\"";
			print_str += o_datetime->print(po);
			print_str += "\"";
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
			break;
		}
		case STRUCT_ADDITION: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(CHILD(i).type() == STRUCT_NEGATE) {
						if(po.spacious) print_str += " ";
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
						else print_str += "-";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i)[0].needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i)[0].print(po, format, colorize, tagtype, ips_n);
					} else {
						if(po.spacious) print_str += " ";
						print_str += "+";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
					}
				} else {
					ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
					print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
				}
			}
			break;
		}
		case STRUCT_NEGATE: {
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			bool b_num = !po.preserve_format && colorize && tagtype == TAG_TYPE_TERMINAL && !ips_n.wrap && (CHILD(0).isNumber() || (CHILD(0).isDivision() && CHILD(0)[0].isNumber()) || CHILD(0).isInverse() || (CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber()));
			if(b_num) print_str += (colorize == 2 ? "\033[0;96m" : "\033[0;36m");
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
			else print_str += "-";
			if(b_num) print_str += "\033[0m";
			ips_n.depth++;
			print_str += CHILD(0).print(po, format, colorize, tagtype, ips_n);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			ips_n.depth++;
			if(!po.preserve_format && SIZE == 2 && (CHILD(0).isNumber() || (CHILD(0).isNegate() && CHILD(0)[0].isNumber())) && CHILD(1).isFunction() && CHILD(1).size() == 1 && CHILD(1).function()->id() == FUNCTION_ID_CIS && CHILD(1).function()->referenceName() == "cis") {
				ips_n.wrap = false;
				print_str += CHILD(0).print(po, format, colorize, tagtype, ips_n);
				print_str += " ";
				print_str += "cis";
				print_str += " ";
				ips_n.wrap = (CHILD(1)[0].size() > 0 && (!CHILD(1)[0].isMultiplication() || CHILD(1)[0].size() != 2 || CHILD(1)[0][1].neededMultiplicationSign(po, ips_n, CHILD(1)[0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE) && (!CHILD(1)[0].isNegate() || (CHILD(1)[0][0].size() > 0 && (!CHILD(1)[0][0].isMultiplication() || CHILD(1)[0][0][1].neededMultiplicationSign(po, ips_n, CHILD(1)[0][0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE))));
				print_str += CHILD(1)[0].print(po, format, colorize, tagtype, ips_n);
				break;
			}
			bool b_units = false;
			bool par_prev = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				if(!po.short_multiplication && i > 0) {
					if(po.spacious) print_str += " ";
					if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
					else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MIDDLEDOT;
					else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
					else print_str += "*";
					if(po.spacious) print_str += " ";
				} else if(i > 0) {
					int i_sign = CHILD(i).neededMultiplicationSign(po, ips_n, *this, i + 1, ips_n.wrap || (CHILD(i).isPower() && CHILD(i)[0].needsParenthesis(po, ips_n, CHILD(i), 1, true, true)), par_prev, true, true);
					if(i_sign == MULTIPLICATION_SIGN_NONE && CHILD(i).isPower() && CHILD(i)[0].isUnit() && po.use_unicode_signs && po.abbreviate_names && CHILD(i)[0].unit() == CALCULATOR->getDegUnit()) {
						PrintOptions po2 = po;
						po2.use_unicode_signs = false;
						i_sign = CHILD(i).neededMultiplicationSign(po2, ips_n, *this, i + 1, ips_n.wrap || (CHILD(i).isPower() && CHILD(i)[0].needsParenthesis(po, ips_n, CHILD(i), 1, true, true)), par_prev, true, true);
					}
					switch(i_sign) {
						case MULTIPLICATION_SIGN_SPACE: {print_str += " "; break;}
						case MULTIPLICATION_SIGN_OPERATOR: {
							if(po.spacious) {
								if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIDOT " ";
								else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += " " SIGN_MIDDLEDOT " ";
								else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIPLICATION " ";
								else print_str += " * ";
								break;
							}
						}
						case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
							if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
							else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT || (po.place_units_separately && po.multiplication_sign == MULTIPLICATION_SIGN_X && CHILD(i).isUnit_exp() && CHILD(i - 1).isUnit_exp())) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MIDDLEDOT;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
							else print_str += "*";
							break;
						}
					}
				}
				if(!b_units && po.place_units_separately && !po.preserve_format && colorize && tagtype == TAG_TYPE_TERMINAL) {
					b_units = true;
					for(size_t i2 = i; i2 < SIZE; i2++) {
						if(!CHILD(i2).isUnit_exp()) {
							b_units = false;
							break;
						}
					}
					if(b_units) print_str += (colorize == 2 ? "\033[0;92m" : "\033[0;32m");
				}
				print_str += CHILD(i).print(po, format, b_units ? 0 : colorize, tagtype, ips_n);
				par_prev = ips_n.wrap;
			}
			if(b_units) print_str += "\033[0m";
			break;
		}
		case STRUCT_INVERSE: {
			ips_n.depth++;
			ips_n.division_depth++;
			ips_n.wrap = false;
			bool b_num = !po.preserve_format && po.division_sign == DIVISION_SIGN_SLASH && CHILD(0).isInteger();
			if(b_num && colorize && tagtype == TAG_TYPE_TERMINAL) print_str += (colorize == 2 ? "\033[0;96m" : "\033[0;36m");
			print_str += m_one.print(po, !b_num && format, b_num ? 0 : colorize, tagtype, ips_n);
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				if(po.spacious && !b_num) print_str += " ";
				print_str += SIGN_DIVISION;
				if(po.spacious && !b_num) print_str += " ";
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += " " SIGN_DIVISION_SLASH " ";
			} else {
				if(po.spacious && !b_num) print_str += " ";
				print_str += "/";
				if(po.spacious && !b_num) print_str += " ";
			}
			if(b_num && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
				print_str += CHILD(0).print(po, false, 0, tagtype, ips_n);
			} else {
				ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
				print_str += CHILD(0).print(po, !b_num && format, b_num ? 0 : colorize, tagtype, ips_n);
			}
			if(b_num && colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
			break;
		}
		case STRUCT_DIVISION: {
			ips_n.depth++;
			ips_n.division_depth++;
			bool b_num = !po.preserve_format && CHILD(0).isInteger() && CHILD(1).isInteger();
			bool b_num2 = b_num && po.division_sign == DIVISION_SIGN_SLASH;
			bool b_units = false;
			if(!b_num && po.place_units_separately && !po.preserve_format) {
				b_units = true;
				if(CHILD(0).isMultiplication()) {
					for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
						if(!CHILD(0)[i2].isUnit_exp()) {
							b_units = false;
							break;
						}
					}
				} else if(!CHILD(0).isUnit_exp()) {
					b_units = false;
				}
				if(b_units) {
					if(CHILD(1).isMultiplication()) {
						for(size_t i2 = 0; i2 < CHILD(1).size(); i2++) {
							if(!CHILD(1)[i2].isUnit_exp()) {
								b_units = false;
								break;
							}
						}
					} else if(!CHILD(1).isUnit_exp()) {
						b_units = false;
					}
				}
			}
			if(colorize && tagtype == TAG_TYPE_TERMINAL) {
				if(b_units) print_str += (colorize == 2 ? "\033[0;92m" : "\033[0;32m");
				else if(b_num2) print_str += (colorize == 2 ? "\033[0;96m" : "\033[0;36m");
			}
			if(b_num && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				ips_n.wrap = CHILD(0).needsParenthesis(po2, ips_n, *this, 1, true, true);
				print_str += CHILD(0).print(po2, !b_units && !b_num2 && format, (b_units || b_num2) ? 0 : colorize, tagtype, ips_n);
			} else {
				ips_n.wrap = !b_units && CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
				print_str += CHILD(0).print(po, !b_units && !b_num2 && format, (b_units || b_num2) ? 0 : colorize, tagtype, ips_n);
			}
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				if(po.spacious && !b_units && !b_num2) print_str += " ";
				print_str += SIGN_DIVISION;
				if(po.spacious && !b_units && !b_num2) print_str += " ";
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += " " SIGN_DIVISION_SLASH " ";
			} else {
				if(po.spacious && !b_units && !b_num2) print_str += " ";
				print_str += "/";
				if(po.spacious && !b_units && !b_num2) print_str += " ";
			}
			if(b_num && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				ips_n.wrap = CHILD(1).needsParenthesis(po2, ips_n, *this, 2, true, true);
				print_str += CHILD(1).print(po2, !b_units && !b_num2 && format, (b_units || b_num2) ? 0 : colorize, tagtype, ips_n);
			} else {
				ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
				print_str += CHILD(1).print(po, !b_units && !b_num2 && format, (b_units || b_num2) ? 0 : colorize, tagtype, ips_n);
			}
			if((b_units || b_num2) && colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
			break;
		}
		case STRUCT_POWER: {
			if(!po.negative_exponents && tagtype == TAG_TYPE_TERMINAL && po.use_unicode_signs && po.place_units_separately && !po.preserve_format && CHILD(0).isUnit() && CHILD(1).isInteger() && CHILD(1).number() >= 2 && CHILD(1).number() <= 9) {
				string s_super;
				if(CHILD(1).number() == 2) s_super = SIGN_POWER_2;
				else if(CHILD(1).number() == 3) s_super = SIGN_POWER_3;
				else if(CHILD(1).number() == 4) s_super = SIGN_POWER_4;
				else if(CHILD(1).number() == 5) s_super = SIGN_POWER_5;
				else if(CHILD(1).number() == 6) s_super = SIGN_POWER_6;
				else if(CHILD(1).number() == 7) s_super = SIGN_POWER_7;
				else if(CHILD(1).number() == 8) s_super = SIGN_POWER_8;
				else if(CHILD(1).number() == 9) s_super = SIGN_POWER_9;
				if(!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (s_super.c_str(), po.can_display_unicode_string_arg)) {
					if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str = (colorize == 2 ? "\033[0;92m" : "\033[0;32m");
					ips_n.wrap = false;
					if(CHILD(0).isUnit() && po.use_unicode_signs && po.abbreviate_names && CHILD(0).unit() == CALCULATOR->getDegUnit()) {
						PrintOptions po2 = po;
						po2.use_unicode_signs = false;
						print_str += CHILD(0).print(po2, false, false, tagtype, ips_n);
					} else {
						print_str += CHILD(0).print(po, false, false, tagtype, ips_n);
					}
					print_str += s_super;
					if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
					break;
				}
			}
			ips_n.depth++;
			ips_n.power_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			bool b_units = po.place_units_separately && !po.preserve_format && colorize && tagtype == TAG_TYPE_TERMINAL && CHILD(0).isUnit();
			if(b_units) print_str = colorize == 2 ? "\033[0;92m" : "\033[0;32m";
			PrintOptions po2 = po;
			if(CHILD(0).isUnit() && po.use_unicode_signs && po.abbreviate_names && CHILD(0).unit() == CALCULATOR->getDegUnit()) po2.use_unicode_signs = false;
			print_str += CHILD(0).print(po2, format, b_units ? 0 : colorize, tagtype, ips_n);
			print_str += "^";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			po2.use_unicode_signs = po.use_unicode_signs;
			po2.show_ending_zeroes = false;
			print_str += CHILD(1).print(po2, format, b_units ? 0 : colorize, tagtype, ips_n);
			if(b_units) print_str += "\033[0m";
			break;
		}
		case STRUCT_COMPARISON: {
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, format, colorize, tagtype, ips_n);
			if(po.spacious) print_str += " ";
			switch(ct_comp) {
				case COMPARISON_EQUALS: {
					if(po.use_unicode_signs && po.interval_display != INTERVAL_DISPLAY_INTERVAL && isApproximate() && containsInterval() && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_ALMOST_EQUAL;
					else print_str += "=";
					break;
				}
				case COMPARISON_NOT_EQUALS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_NOT_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_NOT_EQUAL;
					else print_str += "!=";
					break;
				}
				case COMPARISON_GREATER: {print_str += ">"; break;}
				case COMPARISON_LESS: {print_str += "<"; break;}
				case COMPARISON_EQUALS_GREATER: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
					else print_str += ">=";
					break;
				}
				case COMPARISON_EQUALS_LESS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
					else print_str += "<=";
					break;
				}
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			print_str += CHILD(1).print(po, format, colorize, tagtype, ips_n);
			break;
		}
		case STRUCT_BITWISE_AND: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "&";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "|";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += " ";
					print_str += "xor";
					print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_NOT: {
			print_str = "~";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, format, colorize, tagtype, ips_n);
			break;
		}
		case STRUCT_LOGICAL_AND: {
			ips_n.depth++;
			if(!po.preserve_format && SIZE == 2 && CHILD(0).isComparison() && CHILD(1).isComparison() && CHILD(0).comparisonType() != COMPARISON_EQUALS && CHILD(0).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(1).comparisonType() != COMPARISON_EQUALS && CHILD(1).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(0)[0] == CHILD(1)[0]) {
				ips_n.wrap = CHILD(0)[1].needsParenthesis(po, ips_n, CHILD(0), 2, true, true);
				print_str += CHILD(0)[1].print(po, format, colorize, tagtype, ips_n);
				if(po.spacious) print_str += " ";
				switch(CHILD(0).comparisonType()) {
					case COMPARISON_LESS: {print_str += ">"; break;}
					case COMPARISON_GREATER: {print_str += "<"; break;}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">=";
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<=";
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";

				ips_n.wrap = CHILD(0)[0].needsParenthesis(po, ips_n, CHILD(0), 1, true, true);
				print_str += CHILD(0)[0].print(po, format, colorize, tagtype, ips_n);

				if(po.spacious) print_str += " ";
				switch(CHILD(1).comparisonType()) {
					case COMPARISON_GREATER: {print_str += ">"; break;}
					case COMPARISON_LESS: {print_str += "<"; break;}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">=";
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<=";
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";

				ips_n.wrap = CHILD(1)[1].needsParenthesis(po, ips_n, CHILD(1), 2, true, true);
				print_str += CHILD(1)[1].print(po, format, colorize, tagtype, ips_n);

				break;
			}
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spell_out_logical_operators) {
						print_str += " ";
						print_str += _("and");
						print_str += " ";
					} else {
						if(po.spacious) print_str += " ";
						print_str += "&&";
						if(po.spacious) print_str += " ";
					}
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spell_out_logical_operators) {
						print_str += " ";
						print_str += _("or");
						print_str += " ";
					} else {
						if(po.spacious) print_str += " ";
						print_str += "||";
						if(po.spacious) print_str += " ";
					}
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += " ";
					print_str += "xor";
					print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			print_str = "!";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, format, colorize, tagtype, ips_n);
			break;
		}
		case STRUCT_VECTOR: {
			ips_n.depth++;
			print_str = "[";
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
			}
			print_str += "]";
			break;
		}
		case STRUCT_UNIT: {
			const ExpressionName *ename = &o_unit->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, b_plural, po.use_reference_names || (po.preserve_format && o_unit->isCurrency()), po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(o_prefix) print_str += o_prefix->name(po.abbreviate_names && ename->abbreviation, po.use_unicode_signs, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(ename->suffix && !po.preserve_format && !po.use_reference_names) {
				size_t i = print_str.rfind('_');
				if(i != string::npos && i + 5 <= print_str.length() && print_str.substr(print_str.length() - 4, 4) == "unit") {
					if(i + 5 == print_str.length()) {
						print_str = print_str.substr(0, i);
						if(po.hide_underscore_spaces) gsub("_", " ", print_str);
					} else {
						print_str = print_str.substr(0, print_str.length() - 4);
					}
				}
			}
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			if(colorize && tagtype == TAG_TYPE_TERMINAL) {
				print_str.insert(0, colorize == 2 ? "\033[0;92m" : "\033[0;32m");
				print_str += "\033[0m";
			}
			break;
		}
		case STRUCT_VARIABLE: {
			const ExpressionName *ename = &o_variable->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(ename->suffix && !po.preserve_format && !po.use_reference_names) {
				size_t i = print_str.rfind('_');
				if(i != string::npos && i + 9 <= print_str.length() && print_str.substr(print_str.length() - 8, 8) == "constant") {
					if(i + 9 == print_str.length()) {
						print_str = print_str.substr(0, i);
						if(po.hide_underscore_spaces) gsub("_", " ", print_str);
					} else {
						print_str = print_str.substr(0, print_str.length() - 8);
					}
				}
			}
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			if(colorize && tagtype == TAG_TYPE_TERMINAL) {
				if(o_variable->isKnown()) {
					print_str.insert(0, colorize == 2 ? "\033[0;93m" : "\033[0;33m");
				} else {
					if(format) print_str.insert(0, "\033[3m");
					print_str.insert(0, colorize == 2 ? "\033[0;93m" : "\033[0;33m");
					if(format) print_str += "\033[23m";
				}
				print_str += "\033[0m";
			}
			break;
		}
		case STRUCT_FUNCTION: {
			ips_n.depth++;
			if(o_function->id() == FUNCTION_ID_UNCERTAINTY && SIZE == 3 && CHILD(2).isZero()) {
				MathStructure *mmid = NULL, *munc = NULL;
				if(o_function->id() == FUNCTION_ID_UNCERTAINTY) {
					mmid = &CHILD(0);
					munc = &CHILD(1);
				} else if(CHILD(0)[0].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[1];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				} else if(CHILD(0)[0].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				}
				if(mmid && munc) {
					PrintOptions po2 = po;
					po2.show_ending_zeroes = false;
					po2.number_fraction_format = FRACTION_DECIMAL;
					ips_n.wrap = !CHILD(0).isNumber();
					print_str += CHILD(0).print(po2, format, colorize, tagtype, ips_n);
					print_str += SIGN_PLUSMINUS;
					ips_n.wrap = !CHILD(1).isNumber();
					print_str += CHILD(1).print(po2, format, colorize, tagtype, ips_n);
					break;
				}
			}
			const ExpressionName *ename = &o_function->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			print_str += "(";
			size_t argcount = SIZE;
			if(o_function->id() == FUNCTION_ID_SIGNUM && argcount > 1) {
				argcount = 1;
			} else if(o_function->maxargs() > 0 && o_function->minargs() < o_function->maxargs() && SIZE > (size_t) o_function->minargs()) {
				while(true) {
					string defstr = o_function->getDefaultValue(argcount);
					Argument *arg = o_function->getArgumentDefinition(argcount);
					remove_blank_ends(defstr);
					if(defstr.empty()) break;
					if(CHILD(argcount - 1).isUndefined() && defstr == "undefined") {
						argcount--;
					} else if(argcount > 1 && arg && arg->type() == ARGUMENT_TYPE_SYMBOLIC && defstr == "undefined" && CHILD(argcount - 1) == CHILD(0).find_x_var()) {
						argcount--;
					} else if(CHILD(argcount - 1).isVariable() && (!arg || arg->type() != ARGUMENT_TYPE_TEXT) && defstr == CHILD(argcount - 1).variable()->referenceName()) {
						argcount--;
					} else if(CHILD(argcount - 1).isInteger() && (!arg || arg->type() != ARGUMENT_TYPE_TEXT) && defstr.find_first_not_of(NUMBERS) == string::npos && CHILD(argcount - 1).number() == s2i(defstr)) {
						argcount--;
					} else if(CHILD(argcount - 1).isSymbolic() && arg && arg->type() == ARGUMENT_TYPE_TEXT && CHILD(argcount - 1).symbol() == defstr) {
						argcount--;
					} else {
						break;
					}
					if(argcount == 0 || argcount == (size_t) o_function->minargs()) break;
				}
			}
			for(size_t i = 0; i < argcount; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				if(o_function->id() == FUNCTION_ID_INTERVAL) {
					PrintOptions po2 = po;
					po2.show_ending_zeroes = false;
					print_str += CHILD(i).print(po2, format, colorize, tagtype, ips_n);
				} else {
					print_str += CHILD(i).print(po, format, colorize, tagtype, ips_n);
				}
			}
			print_str += ")";
			break;
		}
		case STRUCT_UNDEFINED: {
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str = (colorize == 2 ? "\033[0;91m" : "\033[0;31m");
			print_str += _("undefined");
			if(colorize && tagtype == TAG_TYPE_TERMINAL) print_str += "\033[0m";
			break;
		}
	}
	if(CALCULATOR->aborted()) {
		if(colorize && tagtype == TAG_TYPE_TERMINAL) {
			print_str = (colorize == 2 ? "\033[0;91m" : "\033[0;31m");
			print_str += CALCULATOR->abortedMessage();
			print_str += "\033[0m";
		} else {
			print_str = CALCULATOR->abortedMessage();
		}
	}
	if(ips.wrap) {
		print_str.insert(0, "(");
		print_str += ")";
	}
	return print_str;
}

