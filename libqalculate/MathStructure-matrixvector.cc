/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
using std::endl;

MathStructure &MathStructure::flattenVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	MathStructure mstruct2;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			CHILD(i).flattenVector(mstruct2);
			for(size_t i2 = 0; i2 < mstruct2.size(); i2++) {
				mstruct.addChild(mstruct2[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
bool MathStructure::rankVector(bool ascending) {
	vector<int> ranked;
	vector<bool> ranked_equals_prev;
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			ComparisonResult cmp = CHILD(index).compare(CHILD(ranked[i]));
			if(COMPARISON_NOT_FULLY_KNOWN(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to rank vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && cmp == COMPARISON_RESULT_GREATER) || cmp == COMPARISON_RESULT_EQUAL || (!ascending && cmp == COMPARISON_RESULT_LESS)) {
				if(cmp == COMPARISON_RESULT_EQUAL) {
					ranked.insert(ranked.begin() + i + 1, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i + 1, true);
				} else {
					ranked.insert(ranked.begin() + i, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i, false);
				}
				b = true;
				break;
			}
		}
		if(!b) {
			ranked.push_back(index);
			ranked_equals_prev.push_back(false);
		}
	}
	int n_rep = 0;
	for(long int i = (long int) ranked.size() - 1; i >= 0; i--) {
		if(CALCULATOR->aborted()) return false;
		if(ranked_equals_prev[i]) {
			n_rep++;
		} else {
			if(n_rep) {
				MathStructure v(i + 1 + n_rep, 1L, 0L);
				v += i + 1;
				v *= MathStructure(1, 2, 0);
				for(; n_rep >= 0; n_rep--) {
					CHILD(ranked[i + n_rep]) = v;
				}
			} else {
				CHILD(ranked[i]).set(i + 1, 1L, 0L);
			}
			n_rep = 0;
		}
	}
	return true;
}
bool MathStructure::sortVector(bool ascending) {
	vector<size_t> ranked_mstructs;
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked_mstructs.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			ComparisonResult cmp = CHILD(index).compare(*v_subs[ranked_mstructs[i]]);
			if(COMPARISON_MIGHT_BE_LESS_OR_GREATER(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to sort vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && COMPARISON_IS_EQUAL_OR_GREATER(cmp)) || (!ascending && COMPARISON_IS_EQUAL_OR_LESS(cmp))) {
				ranked_mstructs.insert(ranked_mstructs.begin() + i, v_order[index]);
				b = true;
				break;
			}
		}
		if(!b) {
			ranked_mstructs.push_back(v_order[index]);
		}
	}
	v_order = ranked_mstructs;
	return true;
}
void MathStructure::flipVector() {
	if(SIZE <= 1) return;
	for(size_t i = 0; i < SIZE / 2; i++) {
		SWAP_CHILDREN(i, SIZE - i - 1);
	}
}
MathStructure &MathStructure::getRange(int start, int end, MathStructure &mstruct) const {
	if(!isVector()) {
		if(start > 1) {
			mstruct.clear();
			return mstruct;
		} else {
			mstruct = *this;
			return mstruct;
		}
	}
	if(start < 1) start = 1;
	else if(start > (long int) SIZE) {
		mstruct.clear();
		return mstruct;
	}
	if(end < (int) 1 || end > (long int) SIZE) end = SIZE;
	else if(end < start) end = start;
	mstruct.clearVector();
	for(; start <= end; start++) {
		mstruct.addChild(CHILD(start - 1));
	}
	return mstruct;
}

void MathStructure::resizeVector(size_t i, const MathStructure &mfill) {
	if(i > SIZE) {
		for(size_t index = SIZE; index < i; index++) {
			if(index % 10002 == 10001 && CALCULATOR->aborted()) return;
			APPEND(mfill);
		}
	} else if(i < SIZE) {
		REDUCE(i)
	}
}

size_t MathStructure::rows() const {
	if(m_type == STRUCT_VECTOR && SIZE == 0) return 0;
	if(m_type != STRUCT_VECTOR || !isMatrix()) return 1;
	return SIZE;
}
size_t MathStructure::columns() const {
	if(m_type == STRUCT_VECTOR && SIZE == 0) return 0;
	if(m_type != STRUCT_VECTOR) return 1;
	if(!isMatrix()) return SIZE;
	return CHILD(0).size();
}
const MathStructure *MathStructure::getElement(size_t row, size_t column) const {
	if(row == 0 || column == 0) return NULL;
	if(m_type != STRUCT_VECTOR) {
		if(row == 1 && column == 1) return this;
		return NULL;
	}
	if(SIZE == 0) return NULL;
	if(row == 1 && !CHILD(0).isVector()) {
		if(column > SIZE) return NULL;
		return &CHILD(column - 1);
	}
	if(row > SIZE || column > CHILD(row - 1).size()) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure *MathStructure::getElement(size_t row, size_t column) {
	if(row == 0 || column == 0) return NULL;
	if(m_type != STRUCT_VECTOR) {
		if(row == 1 && column == 1) return this;
		return NULL;
	}
	if(SIZE == 0) return NULL;
	if(row == 1 && !CHILD(0).isVector()) {
		if(column > SIZE) return NULL;
		return &CHILD(column - 1);
	}
	if(row > SIZE || column > CHILD(row - 1).size()) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure &MathStructure::getArea(size_t r1, size_t c1, size_t r2, size_t c2, MathStructure &mstruct) const {
	size_t r = rows();
	size_t c = columns();
	if(r1 < 1) r1 = 1;
	else if(r1 > r) r1 = r;
	if(c1 < 1) c1 = 1;
	else if(c1 > c) c1 = c;
	if(r2 < 1 || r2 > r) r2 = r;
	else if(r2 < r1) r2 = r1;
	if(c2 < 1 || c2 > c) c2 = c;
	else if(c2 < c1) c2 = c1;
	mstruct.clearMatrix(); mstruct.resizeMatrix(r2 - r1 + 1, c2 - c1 + 1, m_undefined);
	for(size_t index_r = r1; index_r <= r2; index_r++) {
		for(size_t index_c = c1; index_c <= c2; index_c++) {
			mstruct[index_r - r1][index_c - c1] = CHILD(index_r - 1)[index_c - 1];
		}
	}
	return mstruct;
}
MathStructure &MathStructure::rowToVector(size_t r, MathStructure &mstruct) const {
	if(r > rows()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(r < 1) r = 1;
	mstruct = CHILD(r - 1);
	return mstruct;
}
MathStructure &MathStructure::columnToVector(size_t c, MathStructure &mstruct) const {
	if(c > columns()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(c < 1) c = 1;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		mstruct.addChild(CHILD(i)[c - 1]);
	}
	return mstruct;
}
MathStructure &MathStructure::matrixToVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
				mstruct.addChild(CHILD(i)[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
void MathStructure::setElement(const MathStructure &mstruct, size_t row, size_t column) {
	if(row > rows() || column > columns() || row < 1 || column < 1) return;
	CHILD(row - 1)[column - 1] = mstruct;
	CHILD(row - 1).childUpdated(column);
	CHILD_UPDATED(row - 1);
}
void MathStructure::addRows(size_t r, const MathStructure &mfill) {
	if(r == 0) return;
	size_t cols = columns();
	for(size_t i = 0; i < r; i++) {
		APPEND(m_zero)
		LAST.clearVector();
		for(size_t i2 = 0; i2 < cols; i2++) LAST.addChild(mfill);
	}
}
void MathStructure::addColumns(size_t c, const MathStructure &mfill) {
	if(c == 0) return;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < c; i2++) {
				CHILD(i).addChild(mfill);
			}
		}
	}
	CHILDREN_UPDATED;
}
void MathStructure::addRow(const MathStructure &mfill) {
	addRows(1, mfill);
}
void MathStructure::addColumn(const MathStructure &mfill) {
	addColumns(1, mfill);
}
void MathStructure::resizeMatrix(size_t r, size_t c, const MathStructure &mfill) {
	if(r > SIZE) {
		addRows(r - SIZE, mfill);
	} else if(r != SIZE) {
		REDUCE(r);
	}
	size_t cols = columns();
	if(c > cols) {
		addColumns(c - cols, mfill);
	} else if(c != cols) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).resizeVector(c, mfill);
		}
	}
}
bool MathStructure::matrixIsSquare() const {
	return rows() == columns();
}

bool MathStructure::isNumericMatrix() const {
	if(!isMatrix()) return false;
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(index_r).size(); index_c++) {
			if(!CHILD(index_r)[index_c].isNumber() || CHILD(index_r)[index_c].isInfinity()) return false;
		}
	}
	return true;
}

//from GiNaC
int MathStructure::pivot(size_t ro, size_t co, bool symbolic) {

	size_t k = ro;
	if(symbolic) {
		while((k < SIZE) && (CHILD(k)[co].isZero())) ++k;
	} else {
		size_t kmax = k + 1;
		Number mmax(CHILD(kmax)[co].number());
		mmax.abs();
		while(kmax < SIZE) {
			if(CHILD(kmax)[co].number().isNegative()) {
				Number ntmp(CHILD(kmax)[co].number());
				ntmp.negate();
				if(ntmp.isGreaterThan(mmax)) {
					mmax = ntmp;
					k = kmax;
				}
			} else if(CHILD(kmax)[co].number().isGreaterThan(mmax)) {
				mmax = CHILD(kmax)[co].number();
				k = kmax;
			}
			++kmax;
		}
		if(!mmax.isZero()) k = kmax;
	}
	if(k == SIZE) return -1;
	if(k == ro) return 0;

	SWAP_CHILDREN(ro, k)

	return k;

}


//from GiNaC
void determinant_minor(const MathStructure &mtrx, MathStructure &mdet, const EvaluationOptions &eo) {
	size_t n = mtrx.size();
	if(n == 1) {
		mdet = mtrx[0][0];
		return;
	}
	if(n == 2) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.add(mtrx[1][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[0][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		return;
	}
	if(n == 3) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.calculateMultiply(mtrx[2][2], eo);
		mdet.add(mtrx[0][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][2], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][1], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		return;
	}

	std::vector<size_t> Pkey;
	Pkey.reserve(n);
	std::vector<size_t> Mkey;
	Mkey.reserve(n - 1);
	typedef std::map<std::vector<size_t>, class MathStructure> Rmap;
	typedef std::map<std::vector<size_t>, class MathStructure>::value_type Rmap_value;
	Rmap A;
	Rmap B;
	for(size_t r = 0; r < n; ++r) {
		Pkey.erase(Pkey.begin(), Pkey.end());
		Pkey.push_back(r);
		A.insert(Rmap_value(Pkey, mtrx[r][n - 1]));
	}
	for(long int c = n - 2; c >= 0; --c) {
		Pkey.erase(Pkey.begin(), Pkey.end());
		Mkey.erase(Mkey.begin(), Mkey.end());
		for(size_t i = 0; i < n - c; ++i) Pkey.push_back(i);
		size_t fc = 0;
		do {
			mdet.clear();
			for(size_t r = 0; r < n - c; ++r) {
				if (mtrx[Pkey[r]][c].isZero()) continue;
				Mkey.erase(Mkey.begin(), Mkey.end());
				for(size_t i = 0; i < n - c; ++i) {
					if(i != r) Mkey.push_back(Pkey[i]);
				}
				mdet.add(mtrx[Pkey[r]][c], true);
				mdet[mdet.size() - 1].calculateMultiply(A[Mkey], eo);
				if(r % 2) mdet[mdet.size() - 1].calculateNegate(eo);
				mdet.calculateAddLast(eo);
			}
			if(!mdet.isZero()) B.insert(Rmap_value(Pkey, mdet));
			for(fc = n - c; fc > 0; --fc) {
				++Pkey[fc-1];
				if(Pkey[fc - 1] < fc + c) break;
			}
			if(fc < n - c && fc > 0) {
				for(size_t j = fc; j < n - c; ++j) Pkey[j] = Pkey[j - 1] + 1;
			}
		} while(fc);
		A = B;
		B.clear();
	}
	return;
}

//from GiNaC
int MathStructure::gaussianElimination(const EvaluationOptions &eo, bool det) {

	if(!isMatrix()) return 0;
	bool isnumeric = isNumericMatrix();

	size_t m = rows();
	size_t n = columns();
	int sign = 1;

	size_t r0 = 0;
	for(size_t c0 = 0; c0 < n && r0 < m - 1; ++c0) {
		int indx = pivot(r0, c0, true);
		if(indx == -1) {
			sign = 0;
			if(det) return 0;
		}
		if(indx >= 0) {
			if(indx > 0) sign = -sign;
			for(size_t r2 = r0 + 1; r2 < m; ++r2) {
				if(!CHILD(r2)[c0].isZero()) {
					if(isnumeric) {
						Number piv(CHILD(r2)[c0].number());
						piv /= CHILD(r0)[c0].number();
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].number() -= piv * CHILD(r0)[c].number();
						}
					} else {
						MathStructure piv(CHILD(r2)[c0]);
						piv.calculateDivide(CHILD(r0)[c0], eo);
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].add(piv, true);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateMultiply(CHILD(r0)[c], eo);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateNegate(eo);
							CHILD(r2)[c].calculateAddLast(eo);
						}
					}
				}
				for(size_t c = r0; c <= c0; ++c) CHILD(r2)[c].clear();
			}
			if(det) {
				for(size_t c = r0 + 1; c < n; ++c) CHILD(r0)[c].clear();
			}
			++r0;
		}
	}
	for(size_t r = r0 + 1; r < m; ++r) {
		for(size_t c = 0; c < n; ++c) CHILD(r)[c].clear();
	}

	return sign;
}

//from GiNaC
template <class It>
int permutation_sign(It first, It last)
{
	if (first == last)
		return 0;
	--last;
	if (first == last)
		return 0;
	It flag = first;
	int sign = 1;

	do {
		It i = last, other = last;
		--other;
		bool swapped = false;
		while (i != first) {
			if (*i < *other) {
				std::iter_swap(other, i);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*other < *i))
				return 0;
			--i; --other;
		}
		if (!swapped)
			return sign;
		++flag;
		if (flag == last)
			return sign;
		first = flag;
		i = first; other = first;
		++other;
		swapped = false;
		while (i != last) {
			if (*other < *i) {
				std::iter_swap(i, other);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*i < *other))
				return 0;
			++i; ++other;
		}
		if (!swapped)
			return sign;
		last = flag;
		--last;
	} while (first != last);

	return sign;
}

//from GiNaC
MathStructure &MathStructure::determinant(MathStructure &mstruct, const EvaluationOptions &eo) const {

	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The determinant can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}

	if(SIZE == 1) {
		mstruct = CHILD(0)[0];
	} else if(isNumericMatrix()) {

		mstruct.set(1, 1, 0);
		MathStructure mtmp(*this);
		int sign = mtmp.gaussianElimination(eo, true);
		for(size_t d = 0; d < SIZE; ++d) {
			mstruct.number() *= mtmp[d][d].number();
		}
		mstruct.number() *= sign;

	} else {

		typedef std::pair<size_t, size_t> sizet_pair;
		std::vector<sizet_pair> c_zeros;
		for(size_t c = 0; c < CHILD(0).size(); ++c) {
			size_t acc = 0;
			for(size_t r = 0; r < SIZE; ++r) {
				if(CHILD(r)[c].isZero()) ++acc;
			}
			c_zeros.push_back(sizet_pair(acc, c));
		}

		std::sort(c_zeros.begin(), c_zeros.end());
		std::vector<size_t> pre_sort;
		for(std::vector<sizet_pair>::const_iterator i = c_zeros.begin(); i != c_zeros.end(); ++i) {
			pre_sort.push_back(i->second);
		}
		std::vector<size_t> pre_sort_test(pre_sort);

		int sign = permutation_sign(pre_sort_test.begin(), pre_sort_test.end());

		MathStructure result;
		result.clearMatrix();
		result.resizeMatrix(SIZE, CHILD(0).size(), m_zero);

		size_t c = 0;
		for(std::vector<size_t>::const_iterator i = pre_sort.begin(); i != pre_sort.end(); ++i,++c) {
			for(size_t r = 0; r < SIZE; ++r) result[r][c] = CHILD(r)[(*i)];
		}
		mstruct.clear();

		determinant_minor(result, mstruct, eo);

		if(sign != 1) {
			mstruct.calculateMultiply(sign, eo);
		}

	}

	mstruct.mergePrecision(*this);
	return mstruct;

}

MathStructure &MathStructure::permanent(MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The permanent can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}
	if(b_approx) mstruct.setApproximate();
	mstruct.setPrecision(i_precision);
	if(SIZE == 1) {
		if(CHILD(0).size() >= 1) {
			mstruct = CHILD(0)[0];
		}
	} else if(SIZE == 2) {
		mstruct = CHILD(0)[0];
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[1])) {
			mstruct.number() *= CHILD(1)[1].number();
		} else {
			mstruct.calculateMultiply(CHILD(1)[1], eo);
		}
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[0]) && IS_REAL(CHILD(0)[1])) {
			mstruct.number() += CHILD(1)[0].number() * CHILD(0)[1].number();
		} else {
			MathStructure mtmp = CHILD(1)[0];
			mtmp.calculateMultiply(CHILD(0)[1], eo);
			mstruct.calculateAdd(mtmp, eo);
		}
	} else {
		MathStructure mtrx;
		mtrx.clearMatrix();
		mtrx.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			for(size_t index_r2 = 1; index_r2 < SIZE; index_r2++) {
				for(size_t index_c2 = 0; index_c2 < CHILD(index_r2).size(); index_c2++) {
					if(index_c2 > index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2);
					} else if(index_c2 < index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2 + 1);
					}
				}
			}
			MathStructure mdet;
			mtrx.permanent(mdet, eo);
			if(IS_REAL(mdet) && IS_REAL(CHILD(0)[index_c])) {
				mdet.number() *= CHILD(0)[index_c].number();
			} else {
				mdet.calculateMultiply(CHILD(0)[index_c], eo);
			}
			if(IS_REAL(mdet) && IS_REAL(mstruct)) {
				mstruct.number() += mdet.number();
			} else {
				mstruct.calculateAdd(mdet, eo);
			}
		}
	}
	return mstruct;
}
void MathStructure::setToIdentityMatrix(size_t n) {
	clearMatrix();
	resizeMatrix(n, n, m_zero);
	for(size_t i = 0; i < n; i++) {
		CHILD(i)[i] = m_one;
	}
}
MathStructure &MathStructure::getIdentityMatrix(MathStructure &mstruct) const {
	mstruct.setToIdentityMatrix(columns());
	return mstruct;
}

//modified algorithm from eigenmath
bool MathStructure::invertMatrix(const EvaluationOptions &eo) {

	if(!matrixIsSquare()) return false;

	if(isNumericMatrix()) {

		int d, i, j, n = SIZE;

		MathStructure idstruct;
		Number mtmp;
		idstruct.setToIdentityMatrix(n);
		MathStructure mtrx(*this);

		for(d = 0; d < n; d++) {
			if(mtrx[d][d].isZero()) {
				for (i = d + 1; i < n; i++) {
					if(!mtrx[i][d].isZero()) break;
				}
				if(CALCULATOR->aborted()) return false;
				if(i == n) {
					CALCULATOR->error(true, _("Inverse of singular matrix."), NULL);
					return false;
				}

				mtrx[i].ref();
				mtrx[d].ref();
				MathStructure *mptr = &mtrx[d];
				mtrx.setChild_nocopy(&mtrx[i], d + 1);
				mtrx.setChild_nocopy(mptr, i + 1);

				idstruct[i].ref();
				idstruct[d].ref();
				mptr = &idstruct[d];
				idstruct.setChild_nocopy(&idstruct[i], d + 1);
				idstruct.setChild_nocopy(mptr, i + 1);

			}

			mtmp = mtrx[d][d].number();
			mtmp.recip();

			for(j = 0; j < n; j++) {
				if(CALCULATOR->aborted()) return false;
				if(j > d) {
					mtrx[d][j].number() *= mtmp;
				}

				idstruct[d][j].number() *= mtmp;

			}

			for(i = 0; i < n; i++) {

				if(i == d) continue;

				mtmp = mtrx[i][d].number();
				mtmp.negate();

				for(j = 0; j < n; j++) {
					if(CALCULATOR->aborted()) return false;
					if(j > d) {
						mtrx[i][j].number() += mtrx[d][j].number() * mtmp;
					}

					idstruct[i][j].number() += idstruct[d][j].number() * mtmp;

				}
			}
		}
		set_nocopy(idstruct);
		MERGE_APPROX_AND_PREC(idstruct)
	} else {
		MathStructure *mstruct = new MathStructure();
		determinant(*mstruct, eo);
		mstruct->calculateInverse(eo);
		adjointMatrix(eo);
		multiply_nocopy(mstruct, true);
		calculateMultiplyLast(eo);
	}
	return true;
}

bool MathStructure::adjointMatrix(const EvaluationOptions &eo) {
	if(!matrixIsSquare()) return false;
	if(SIZE == 1) {CHILD(0)[0].set(1, 1, 0); return true;}
	MathStructure msave(*this);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			if(CALCULATOR->aborted()) return false;
			msave.cofactor(index_r + 1, index_c + 1, CHILD(index_r)[index_c], eo);
		}
	}
	transposeMatrix();
	return true;
}
bool MathStructure::transposeMatrix() {
	if(SIZE > 0 && CHILD(0).size() == 1) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).setToChild(1, true);
		}
		return true;
	}
	MathStructure msave(*this);
	resizeMatrix(CHILD(0).size(), SIZE, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			if(CALCULATOR->aborted()) return false;
			CHILD(index_r)[index_c] = msave[index_c][index_r];
		}
	}
	return true;
}
MathStructure &MathStructure::cofactor(size_t r, size_t c, MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(r < 1) r = 1;
	if(c < 1) c = 1;
	if(SIZE == 1 || r > SIZE || c > CHILD(0).size()) {
		mstruct = m_undefined;
		return mstruct;
	}
	r--; c--;
	mstruct.clearMatrix();
	mstruct.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		if(index_r != r) {
			for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
				if(index_c > c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c - 1] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c - 1] = CHILD(index_r)[index_c];
					}
				} else if(index_c < c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c] = CHILD(index_r)[index_c];
					}
				}
			}
		}
	}
	MathStructure mstruct2;
	mstruct = mstruct.determinant(mstruct2, eo);
	if((r + c) % 2 == 1) {
		mstruct.calculateNegate(eo);
	}
	return mstruct;
}

bool calculate_userfunctions(MathStructure &m, const MathStructure &x_mstruct, const EvaluationOptions &eo, bool b_vector, size_t depth) {
	if(!check_recursive_function_depth(depth)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(calculate_userfunctions(m[i], x_mstruct, eo, b_vector, depth + 1)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(m.isFunction()) {
		if(!m.contains(x_mstruct, true) && !m.containsFunctionId(FUNCTION_ID_RAND, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RANDN, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RAND_POISSON, true, true, true)) {
			if(m.calculateFunctions(eo, false)) {
				b_ret = true;
				calculate_userfunctions(m, x_mstruct, eo, b_vector, depth + 1);
			}
		} else if(m.function()->subtype() == SUBTYPE_USER_FUNCTION && m.function()->condition().empty()) {
			bool b = true;
			for(size_t i = 0; i < ((UserFunction*) m.function())->countSubfunctions(); i++) {
				if(((UserFunction*) m.function())->subfunctionPrecalculated(i + 1)) {
					b = false;
					break;
				}
			}
			for(size_t i = 0; b && i < m.size(); i++) {
				Argument *arg = m.function()->getArgumentDefinition(i + 1);
				if(arg && arg->tests() && (arg->type() != ARGUMENT_TYPE_FREE || !arg->getCustomCondition().empty() || arg->rationalPolynomial() || arg->zeroForbidden() || (arg->handlesVector() && m[i].isVector())) && m[i].contains(x_mstruct, true)) {
					b = false;
					break;
				}
			}
			if(b && m.calculateFunctions(eo, false)) {
				calculate_userfunctions(m, x_mstruct, eo, false, depth + 1);
				b_ret = true;
			}
		} else if(b_vector && ((m.function()->id() == FUNCTION_ID_DIFFERENTIATE && (m.size() < 3 || m[3].isUndefined())) || (m.function()->id() == FUNCTION_ID_INTEGRATE && (m.size() < 3 || (m[1].isUndefined() && m[2].isUndefined()))))) {
			size_t i_x = 1;
			if(m.function()->id() == FUNCTION_ID_INTEGRATE) i_x = 3;
			bool b = m.size() > i_x && m[i_x] == x_mstruct;
			if(!b && m.size() > 0 && (m.size() <= i_x || m[i_x].isUndefined())) {
				if(m[0].find_x_var() == x_mstruct) {
					MathStructure mtest(m[0]);
					mtest.replace(x_mstruct, m_zero);
					b = mtest.find_x_var().isUndefined();
				}
			}
			if(b && m.calculateFunctions(eo, false, true)) {
				if(i_x == 3) m.replace(CALCULATOR->getVariableById(VARIABLE_ID_C), m_zero);
				b_ret = true;
			}
		}
	}
	return b_ret;
}
bool find_legal_point(const MathStructure &m, const MathStructure &x_mstruct, MathStructure &mx, const MathStructure &min, const MathStructure &max, const EvaluationOptions &eo, bool b_number) {
	if(m.isFunction() && (m.function()->id() == FUNCTION_ID_RE || m.function()->id() == FUNCTION_ID_IM || m.function()->id() == FUNCTION_ID_ABS)) return false;
	if((b_number && ((m.isPower() && m[1].isNumber() && m[1].number().isReal() && !m[1].number().isInteger() && m[0].contains(x_mstruct, true) > 0) || (m.isFunction() && m.size() > 0 && (m.function()->id() == FUNCTION_ID_ACOS || m.function()->id() == FUNCTION_ID_ASIN || m.function()->id() == FUNCTION_ID_ACOSH || (m.function()->id() == FUNCTION_ID_LAMBERT_W && (m.size() == 1 || m[1].isZero())))))) || (!b_number && m.isFunction() && m.size() > 0 && m.isFunction() && m.function()->id() == FUNCTION_ID_ROOT && m.size() >= 2 && m[1].representsEven())) {
		MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_SECANT_METHOD);
		if(f) {
			MathStructure msolve(f, NULL);
			msolve.addChild(m[0]);
			if(m.isFunction() && (m.function()->id() == FUNCTION_ID_ACOS || m.function()->id() == FUNCTION_ID_ASIN || m.function()->id() == FUNCTION_ID_ACOSH)) {
				if(m.function()->id() != FUNCTION_ID_ACOSH) msolve[0].transformById(FUNCTION_ID_ABS);
				msolve[0] -= 1;
			} else if(m.isFunction() && m.function()->id() == FUNCTION_ID_LAMBERT_W) {
				msolve[0] += CALCULATOR->getVariableById(VARIABLE_ID_E);
				msolve[0].last().inverse();
			}
			msolve.addChild(min);
			msolve.addChild(max);
			msolve.addChild(x_mstruct);
			msolve.addChild(Number(-10, 1, 0));
			msolve.addChild(Number(10, 1, 0));
			msolve.calculateFunctions(eo, true);
			if(msolve.isNumber() && msolve.number() > min.number() && msolve.number() < max.number()) {
				mx = msolve;
				return true;
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(find_legal_point(m[i], x_mstruct, mx, min, max, eo, b_number)) return true;
	}
	return false;
}
bool find_zero_point(const MathStructure &m, const MathStructure &x_mstruct, MathStructure &mx, const MathStructure &min, const MathStructure &max, const EvaluationOptions &eo) {
	MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_SECANT_METHOD);
	if(f) {
		MathStructure msolve(f, NULL);
		msolve.addChild(m);
		msolve.addChild(min);
		msolve.addChild(max);
		msolve.addChild(x_mstruct);
		msolve.addChild(Number(-10, 1, 0));
		msolve.addChild(Number(10, 1, 0));
		msolve.calculateFunctions(eo, true);
		if(msolve.isNumber() && msolve.number() > min.number() && msolve.number() < max.number()) {
			mx = msolve;
			mx.number().setApproximate(false);
			mx.setApproximate(false);
			return true;
		}
	}
	return false;
}
void generate_plotvector(const MathStructure &m, MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure &x_vector, MathStructure &y_vector, const EvaluationOptions &eo2) {
	EvaluationOptions eo = eo2;
	eo.allow_complex = true;
	MathStructure x_value(min);
	MathStructure y_value;
	y_vector.clearVector();
	x_vector.clearVector();
	if(min != max) {
		MathStructure mtest(max);
		mtest.calculateSubtract(min, eo);
		if(!step.isZero()) mtest.calculateDivide(step, eo);
		mtest.eval(eo);
		if(step.isZero() || !mtest.isNumber() || mtest.number().isNegative()) {
			CALCULATOR->error(true, _("The selected min, max and step size do not result in a positive, finite number of data points"), NULL);
			return;
		} else if(mtest.number().isGreaterThan(1000000)) {
			CALCULATOR->error(true, _("Too many data points"), NULL);
			return;
		}
		mtest.number().round();
		unsigned int steps = mtest.number().uintValue();
		y_vector.resizeVector(steps, m_zero);
		x_vector.resizeVector(steps, m_zero);
	}
	MathStructure mthis(m);
	mthis.unformat();
	calculate_userfunctions(mthis, x_mstruct, eo, true);
	ComparisonResult cr = max.compare(x_value);
	size_t i = 0;
	while(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
		if(i >= x_vector.size()) x_vector.addChild(x_value);
		else x_vector[i] = x_value;
		y_value = mthis;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		if(!eo2.allow_complex && y_value.isNumber() && y_value.number().hasImaginaryPart()) {
			if(testComplexZero(&y_value.number(), y_value.number().internalImaginary())) y_value.number().clearImaginary();
			else y_value.setUndefined();
		}
		if(i >= y_vector.size()) y_vector.addChild(y_value);
		else y_vector[i] = y_value;
		if(x_value.isNumber()) x_value.number().add(step.number());
		else x_value.calculateAdd(step, eo);
		cr = max.compare(x_value);
		if(CALCULATOR->aborted()) {
			break;
		}
		i++;
	}
	y_vector.resizeVector(i, m_zero);
	x_vector.resizeVector(i, m_zero);
}
void generate_plotvector(const MathStructure &m, MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure &x_vector, MathStructure &y_vector, const EvaluationOptions &eo2, bool adaptive) {
	EvaluationOptions eo = eo2;
	eo.allow_complex = true;
	if(steps < 1) {
		steps = 1;
	}
	MathStructure x_value(min);
	MathStructure y_value;
	y_vector.clearVector();
	x_vector.clearVector();
	if(steps > 1000000) {
		CALCULATOR->error(true, _("Too many data points"), NULL);
		return;
	}
	if(adaptive && (!min.isNumber() || !max.isNumber())) adaptive = false;
	if(adaptive) steps = steps / 2 + steps % 2;
	MathStructure step(max);
	step.calculateSubtract(min, eo);
	if(steps != 1) step.calculateDivide(steps - 1, eo);
	step.eval(eo);
	if(!step.isNumber() || step.number().isNegative()) {
		CALCULATOR->error(true, _("The selected min and max do not result in a positive, finite number of data points"), NULL);
		return;
	}
	MathStructure ydiff;
	if(adaptive) ydiff.resizeVector(steps, m_zero);
	Number ydiff_total;
	Number ydiff_real;
	y_vector.resizeVector(steps, m_zero);
	x_vector.resizeVector(steps, m_zero);
	MathStructure mthis(m);
	mthis.unformat();
	calculate_userfunctions(mthis, x_mstruct, eo, true);
	MathStructure meval;
	MathStructure mdiff;
	int prev_illegal = -1;
	int prev_diff_sgn = 0;
	for(int i = 0; i < steps; i++) {
		y_value = mthis;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		x_vector[i] = x_value;
		y_vector[i] = y_value;
		int find_x = 0;
		if(y_value.isNumber() && !y_value.number().includesInfinity()) {
			if(y_value.number().hasImaginaryPart() && testComplexZero(&y_value.number(), y_value.number().internalImaginary())) y_value.number().clearImaginary();
			if(y_value.number().hasImaginaryPart()) {
				if(prev_illegal == 0) find_x = 1;
				if(eo2.allow_complex && adaptive && (prev_illegal == 0 || prev_illegal == 1)) {
					ydiff[i].number() = y_vector[i].number().imaginaryPart();
					ydiff[i].number() -= y_vector[i - 1].number().imaginaryPart();
					ydiff[i].number().abs();
					if(y_value.number().hasRealPart()) {
						ydiff_real = y_vector[i].number().realPart();
						ydiff_real -= y_vector[i - 1].number().realPart();
						ydiff_real.abs();
						if(ydiff_real > ydiff[i].number()) ydiff[i].number() = ydiff_real;
					}
					ydiff_total += ydiff[i].number();
				}
				prev_illegal = 1;
				prev_diff_sgn = 0;
			} else {
				if(prev_illegal == 0 && adaptive) {
					ydiff[i].number() = y_vector[i].number();
					ydiff[i].number() -= y_vector[i - 1].number();
					if(prev_diff_sgn != 0 && (ydiff[i].number().isZero() || ((prev_diff_sgn > 0) != ydiff[i].number().isPositive()))) {
						if(mdiff.isZero()) {
							mdiff = mthis;
							CALCULATOR->beginTemporaryStopMessages();
							mdiff.differentiate(x_mstruct, eo);
							if(mdiff.containsFunctionId(FUNCTION_ID_DIFFERENTIATE)) {
								mdiff.setUndefined();
							} else {
								mdiff.eval(eo);
							}
							CALCULATOR->endTemporaryStopMessages();
						}
						ydiff[i].number().abs();
						MathStructure x_value2, y_value2;
						if(!mdiff.isUndefined() && find_zero_point(mdiff, x_mstruct, x_value2, i >= 2 && !ydiff[i].number().isZero() && y_vector[i - 2].isNumber() && y_vector[i - 2].number().isReal() ? x_vector[i - 2] : x_vector[i - 1], x_value, eo)) {
							int new_i = i;
							if(i >= 2 && x_value2.number() < x_vector[i - 1].number()) {
								new_i = i - 1;
							}
							y_value2 = mthis;
							y_value2.replace(x_mstruct, x_value2);
							y_value2.eval(eo);
							if(y_value2.isNumber() && y_value2.number().hasImaginaryPart() && testComplexZero(&y_value2.number(), y_value2.number().internalImaginary())) y_value2.number().clearImaginary();
							x_vector.insertChild(x_value2, new_i + 1);
							y_vector.insertChild(y_value2, new_i + 1);
							ydiff.insertChild(m_zero, new_i + 1);
							if(!y_value2.isNumber() || !y_value2.number().isReal()) {
								ydiff[new_i + 1].number() /= 2;
								ydiff[new_i].number() = ydiff[new_i + 1].number();
								ydiff_total += ydiff[i].number();
							} else if(new_i == i - 1) {
								ydiff_total -= ydiff[i - 1].number();
								ydiff[i - 1].number() = y_vector[i - 2].number();
								ydiff[i - 1].number() -= y_value2.number();
								ydiff[i - 1].number().abs();
								ydiff_total += ydiff[i - 1].number();
								ydiff[i].number() = y_value.number();
								ydiff[i].number() -= y_value2.number();
								ydiff[i].number().abs();
								ydiff_total += ydiff[i].number();
							} else {
								ydiff[i].number() = y_value2.number();
								ydiff[i].number() -= y_vector[i - 1].number();
								ydiff[i].number().abs();
								ydiff_total += ydiff[i].number();
								ydiff[i + 1].number() = y_value.number();
								ydiff[i + 1].number() -= y_value2.number();
								ydiff[i + 1].number().abs();
							}
							i++;
							steps++;
						}
						prev_diff_sgn = 0;
					} else if(ydiff[i].number().isPositive()) {
						prev_diff_sgn = 1;
					} else if(ydiff[i].number().isNegative()) {
						prev_diff_sgn = -1;
						ydiff[i].number().abs();
					} else {
						prev_diff_sgn = 0;
					}
					ydiff_total += ydiff[i].number();
				} else if(eo2.allow_complex && adaptive && prev_illegal == 1) {
					ydiff[i].number() = y_vector[i - 1].number().imaginaryPart();
					ydiff_real = y_vector[i].number();
					ydiff_real -= y_vector[i - 1].number().realPart();
					ydiff_real.abs();
					if(ydiff_real > ydiff[i].number()) ydiff[i].number() = ydiff_real;
					ydiff_total += ydiff[i].number();
				} else {
					prev_diff_sgn = 0;
				}
				if(prev_illegal > 0 && y_value.number().isReal()) find_x = 2;
				else prev_illegal = 0;
			}
		} else {
			if(prev_illegal == 0) find_x = 1;
			prev_illegal = 2;
			prev_diff_sgn = 0;
		}
		if(find_x > 0 && x_value.isNumber() && step.isNumber()) {
			if(meval.isZero()) {
				CALCULATOR->beginTemporaryStopMessages();
				meval = mthis;
				meval.eval(eo);
				CALCULATOR->endTemporaryStopMessages();
			}
			MathStructure x_value2;
			if(find_legal_point(meval, x_mstruct, x_value2, x_vector[i - 1], x_value, eo, prev_illegal == 1)) {
				if(x_value2.number().precision() > 0) x_value2.number() += step.number() * Number(find_x == 1 ? -1 : 1, 1, -x_value2.number().precision());
				if(x_value2.number() > x_vector[i - 1].number() && x_value2.number() < x_vector[i].number()) {
					y_value = mthis;
					y_value.replace(x_mstruct, x_value2);
					y_value.eval(eo);
					if(y_value.isNumber()) {
						if(y_value.number().hasImaginaryPart() && testComplexZero(&y_value.number(), y_value.number().internalImaginary())) y_value.number().clearImaginary();
						if(y_value.number().isReal()) {
							i++;
							steps++;
							x_vector.insertChild(x_value2, i);
							y_vector.insertChild(y_value, i);
							if(adaptive) {
								ydiff.insertChild(m_zero, i);
								ydiff_total -= ydiff[i].number();
								if(find_x == 1) {
									ydiff[i - 1].number() = y_value.number();
									ydiff[i - 1].number() -= y_vector[i - 2].number();
									ydiff[i - 1].number().abs();
									if(eo2.allow_complex && y_vector[i].isNumber() && !y_vector[i].number().includesInfinity()) {
										ydiff[i].number() = y_vector[i].number().imaginaryPart();
										ydiff_real = y_vector[i].number().realPart();
										ydiff_real -= y_value.number();
										ydiff_real.abs();
										if(ydiff_real > ydiff[i].number()) ydiff[i].number() = ydiff_real;
									}
								} else {
									if(eo2.allow_complex && y_vector[i - 2].isNumber() && !y_vector[i - 2].number().includesInfinity()) {
										ydiff[i - 1].number() = y_vector[i - 2].number().imaginaryPart();
										ydiff[i - 1].number().abs();
										ydiff_real = y_value.number().realPart();
										ydiff_real -= y_vector[i - 2].number().realPart();
										ydiff_real.abs();
										if(ydiff_real > ydiff[i - 1].number()) ydiff[i - 1].number() = ydiff_real;
									}
									ydiff[i].number() = y_vector[i].number();
									ydiff[i].number() -= y_value.number();
									ydiff[i].number().abs();
								}
								ydiff_total += ydiff[i].number();
								ydiff_total += ydiff[i - 1].number();
							}
						}
					}
				}
			}
		}
		if(find_x == 2) prev_illegal = 0;
		if(i == steps - 1) {}
		else if(i + 2 == steps) x_value = max;
		else if(x_value.isNumber()) x_value.number().add(step.number());
		else x_value.calculateAdd(step, eo);
		if(CALCULATOR->aborted()) {
			y_vector.resizeVector(i, m_zero);
			x_vector.resizeVector(i, m_zero);
			break;
		}
	}
	if(!eo2.allow_complex) {
		for(size_t i = 0; i < y_vector.size(); i++) {
			if(y_vector[i].isNumber() && !y_vector[i].number().isReal()) y_vector[i].setUndefined();
		}
	}
	if(adaptive) {
		if(ydiff_total.isZero()) {
			x_value = x_vector[0];
			x_value.number() += step.number() / 2;
			for(int i = 1; i < (int) x_vector.size(); i += 2) {
				if(CALCULATOR->aborted()) break;
				x_vector.insertChild(x_value, i);
				y_value = mthis;
				y_value.replace(x_mstruct, x_value);
				y_value.eval(eo);
				y_vector.insertChild(y_value, i);
				x_value.number() += step.number();
			}
		} else {
			Number new_step_size;
			for(int i = 1; i < (int) x_vector.size(); i++) {
				if(CALCULATOR->aborted()) break;
				int new_steps = 0;
				if(!ydiff[i].number().isZero()) {
					ydiff[i].number() /= ydiff_total;
					ydiff[i].number() *= (steps - 1);
					ydiff[i].number().round();
					new_steps = ydiff[i].number().intValue();
				}
				if(new_steps == 1) {
					x_value = x_vector[i];
					x_value.number() += x_vector[i - 1].number();
					x_value.number() /= 2;
					x_vector.insertChild(x_value, i + 1);
					y_value = mthis;
					y_value.replace(x_mstruct, x_value);
					y_value.eval(eo);
					y_vector.insertChild(y_value, i + 1);
					ydiff.insertChild(m_zero, i + 1);
					i++;
				} else if(new_steps > 0) {
					new_step_size = x_vector[i].number();
					new_step_size -= x_vector[i - 1].number();
					new_step_size /= (new_steps + 1);
					x_value = x_vector[i - 1].number();
					for(int i2 = 0; i2 < new_steps; i2++) {
						if(CALCULATOR->aborted()) break;
						x_value.number() += new_step_size;
						x_vector.insertChild(x_value, i + i2 + 1);
						y_value = mthis;
						y_value.replace(x_mstruct, x_value);
						y_value.eval(eo);
						y_vector.insertChild(y_value, i + i2 + 1);
						ydiff.insertChild(m_zero, i + i2 + 1);
					}
					i += new_steps;
				}
			}
		}
	}
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector, const EvaluationOptions &eo) const {
	if(steps < 1) {
		steps = 1;
	}
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	if(steps > 1000000) {
		CALCULATOR->error(true, _("Too many data points"), NULL);
		return y_vector;
	}
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure step(max);
	step.calculateSubtract(min, eo);
	if(steps != 1) step.calculateDivide(steps - 1, eo);
	step.eval(eo);
	CALCULATOR->endTemporaryStopMessages();
	if(!step.isNumber() || step.number().isNegative()) {
		CALCULATOR->error(true, _("The selected min and max do not result in a positive, finite number of data points"), NULL);
		return y_vector;
	}
	y_vector.resizeVector(steps, m_zero);
	if(x_vector) x_vector->resizeVector(steps, m_zero);
	MathStructure mthis(*this);
	mthis.unformat();
	calculate_userfunctions(mthis, x_mstruct, eo, true);
	for(int i = 0; i < steps; i++) {
		if(x_vector) {
			(*x_vector)[i] = x_value;
		}
		y_value = mthis;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		y_vector[i] = y_value;
		if(i == steps - 1) {}
		else if(i + 2 == steps) x_value = max;
		else if(x_value.isNumber()) x_value.number().add(step.number());
		else x_value.calculateAdd(step, eo);
		if(CALCULATOR->aborted()) {
			return y_vector;
		}
	}
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step_pre, MathStructure *x_vector, const EvaluationOptions &eo) const {
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	MathStructure step;
	if(!step_pre.contains(x_mstruct) && !step_pre.containsFunctionId(FUNCTION_ID_RAND, true, true, true) && !step_pre.containsFunctionId(FUNCTION_ID_RANDN, true, true, true) && !step_pre.containsFunctionId(FUNCTION_ID_RAND_POISSON, true, true, true)) {
		step = step_pre;
		step.eval(eo);
		if(min != max) {
			CALCULATOR->beginTemporaryStopMessages();
			MathStructure mtest(max);
			mtest.calculateSubtract(min, eo);
			if(!step.isZero()) mtest.calculateDivide(step, eo);
			mtest.eval(eo);
			if(!step.isZero() && mtest.isNumber() && mtest.number().isNegative()) {
				step.negate();
				mtest = max;
				mtest.calculateSubtract(min, eo);
				if(!step.isZero()) mtest.calculateDivide(step, eo);
				mtest.eval(eo);
			}
			CALCULATOR->endTemporaryStopMessages();
			if(step.isZero() || !mtest.isNumber() || mtest.number().isNegative()) {
				CALCULATOR->error(true, _("The selected min, max and step size do not result in a positive, finite number of data points"), NULL);
				return y_vector;
			} else if(mtest.number().isGreaterThan(1000000)) {
				CALCULATOR->error(true, _("Too many data points"), NULL);
				return y_vector;
			}
			mtest.number().round();
			unsigned int steps = mtest.number().uintValue();
			y_vector.resizeVector(steps, m_zero);
			if(x_vector) x_vector->resizeVector(steps, m_zero);
		}
	}
	ComparisonResult cr = min.compare(max);
	if(cr == COMPARISON_RESULT_EQUAL) {
		y_vector.addChild(*this);
		y_vector[0].replace(x_mstruct, x_value);
		y_vector[0].eval(eo);
		if(x_vector) x_vector->addChild(x_value);
		return y_vector;
	}
	MathStructure mthis(*this);
	mthis.unformat();
	calculate_userfunctions(mthis, x_mstruct, eo, true);
	bool b_neg = (cr == COMPARISON_RESULT_LESS);
	cr = max.compare(x_value);
	size_t i = 0;
	while((!b_neg && COMPARISON_IS_EQUAL_OR_LESS(cr)) || (b_neg && COMPARISON_IS_EQUAL_OR_GREATER(cr))) {
		if(x_vector) {
			if(i >= x_vector->size()) x_vector->addChild(x_value);
			else (*x_vector)[i] = x_value;
		}
		y_value = mthis;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		if(i >= y_vector.size()) y_vector.addChild(y_value);
		else y_vector[i] = y_value;
		if(step.isZero()) {
			step = step_pre;
			step.replace(x_mstruct, x_value);
			step.eval(eo);
			x_value.calculateAdd(step, eo);
			step.clear();
		} else if(x_value.isNumber() && step.isNumber()) {
			x_value.number().add(step.number());
		} else {
			x_value.calculateAdd(step, eo);
		}
		cr = max.compare(x_value);
		if(CALCULATOR->aborted()) {
			y_vector.resizeVector(i, m_zero);
			if(x_vector) x_vector->resizeVector(i, m_zero);
			return y_vector;
		}
		i++;
	}
	y_vector.resizeVector(i, m_zero);
	if(x_vector) x_vector->resizeVector(i, m_zero);
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &x_vector, const EvaluationOptions &eo) const {
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	MathStructure mthis(*this);
	mthis.unformat();
	calculate_userfunctions(mthis, x_mstruct, eo, true);
	for(size_t i = 1; i <= x_vector.countChildren(); i++) {
		if(CALCULATOR->aborted()) {
			y_vector.clearVector();
			return y_vector;
		}
		y_value = mthis;
		y_value.replace(x_mstruct, x_vector.getChild(i));
		y_value.eval(eo);
		y_vector.addChild(y_value);
	}
	return y_vector;
}

