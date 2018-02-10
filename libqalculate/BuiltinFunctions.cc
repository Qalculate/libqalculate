/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

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

#include <sstream>
#include <time.h>
#include <limits>

#define FR_FUNCTION(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2R(FUNC)	Number nr(vargs[1].number()); if(!nr.FUNC(vargs[0].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}

#define REPRESENTS_FUNCTION(x, y) x::x() : MathFunction(#y, 1) {} int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {mstruct = vargs[0]; mstruct.eval(eo); if(mstruct.y()) {mstruct.clear(); mstruct.number().setTrue();} else {mstruct.clear(); mstruct.number().setFalse();} return 1;}

#define IS_NUMBER_FUNCTION(x, y) x::x() : MathFunction(#y, 1) {} int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {mstruct = vargs[0]; if(!mstruct.isNumber()) mstruct.eval(eo); if(mstruct.isNumber() && mstruct.number().y()) {mstruct.number().setTrue();} else {mstruct.clear(); mstruct.number().setFalse();} return 1;}

#define NON_COMPLEX_NUMBER_ARGUMENT(i)				NumberArgument *arg_non_complex##i = new NumberArgument(); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(i)		NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define RATIONAL_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_rational##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_rational##i->setRationalNumber(true); setArgumentDefinition(i, arg_rational##i);
#define RATIONAL_POLYNOMIAL_ARGUMENT(i)				Argument *arg_poly##i = new Argument(); arg_poly##i->setRationalPolynomial(true); setArgumentDefinition(i, arg_poly##i);


VectorFunction::VectorFunction() : MathFunction("vector", -1) {
}
int VectorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs;
	mstruct.setType(STRUCT_VECTOR);
	return 1;
}
MatrixFunction::MatrixFunction() : MathFunction("matrix", 3) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(3, new VectorArgument());
}
int MatrixFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t rows = (size_t) vargs[0].number().uintValue();
	size_t columns = (size_t) vargs[1].number().uintValue();
	mstruct.clearMatrix(); mstruct.resizeMatrix(rows, columns, m_zero);
	size_t r = 1, c = 1;
	for(size_t i = 0; i < vargs[2].size(); i++) {
		if(r > rows || c > columns) {
			CALCULATOR->error(false, _("Too many elements (%s) for the dimensions (%sx%s) of the matrix."), i2s(vargs[2].size()).c_str(), i2s(rows).c_str(), i2s(columns).c_str(), NULL);
			break;
		}
		mstruct[r - 1][c - 1] = vargs[2][i];	
		if(c == columns) {
			c = 1;
			r++;
		} else {
			c++;
		}
	}
	return 1;
}
RankFunction::RankFunction() : MathFunction("rank", 1, 2) {
	setArgumentDefinition(1, new VectorArgument(""));
	setArgumentDefinition(2, new BooleanArgument(""));
	setDefaultValue(2, "1");
}
int RankFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	return mstruct.rankVector(vargs[1].number().getBoolean());
}
SortFunction::SortFunction() : MathFunction("sort", 1, 2) {
	setArgumentDefinition(1, new VectorArgument(""));
	setArgumentDefinition(2, new BooleanArgument(""));
	setDefaultValue(2, "1");
}
int SortFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	return mstruct.sortVector(vargs[1].number().getBoolean());
}
MergeVectorsFunction::MergeVectorsFunction() : MathFunction("mergevectors", 1, -1) {
	setArgumentDefinition(1, new VectorArgument(""));
	setArgumentDefinition(2, new VectorArgument(""));
}
int MergeVectorsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.clearVector();
	for(size_t i = 0; i < vargs.size(); i++) {
		if(CALCULATOR->aborted()) return 0;
		if(vargs[i].isVector()) {
			for(size_t i2 = 0; i2 < vargs[i].size(); i2++) {
				mstruct.addChild(vargs[i][i2]);
			}
		} else {
			mstruct.addChild(vargs[i]);
		}
	}
	return 1;
}
MatrixToVectorFunction::MatrixToVectorFunction() : MathFunction("matrix2vector", 1) {
	setArgumentDefinition(1, new MatrixArgument());
}
int MatrixToVectorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].matrixToVector(mstruct);
	return 1;
}
RowFunction::RowFunction() : MathFunction("row", 2) {
	setArgumentDefinition(1, new MatrixArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int RowFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t row = (size_t) vargs[1].number().uintValue();
	if(row > vargs[0].rows()) {
		CALCULATOR->error(true, _("Row %s does not exist in matrix."), vargs[1].print().c_str(), NULL);
		return 0;
	}
	vargs[0].rowToVector(row, mstruct);
	return 1;
}
ColumnFunction::ColumnFunction() : MathFunction("column", 2) {
	setArgumentDefinition(1, new MatrixArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int ColumnFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t col = (size_t) vargs[1].number().uintValue();
	if(col > vargs[0].columns()) {
		CALCULATOR->error(true, _("Column %s does not exist in matrix."), vargs[1].print().c_str(), NULL);
		return 0;
	}
	vargs[0].columnToVector(col, mstruct);
	return 1;
}
RowsFunction::RowsFunction() : MathFunction("rows", 1) {
	setArgumentDefinition(1, new MatrixArgument(""));
}
int RowsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) vargs[0].rows();
	return 1;
}
ColumnsFunction::ColumnsFunction() : MathFunction("columns", 1) {
	setArgumentDefinition(1, new MatrixArgument(""));
}
int ColumnsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) vargs[0].columns();
	return 1;
}
ElementsFunction::ElementsFunction() : MathFunction("elements", 1) {
	setArgumentDefinition(1, new VectorArgument(""));
}
int ElementsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].isMatrix()) {
		mstruct = (int) (vargs[0].rows() * vargs[0].columns());
	} else {
		mstruct = (int) vargs[0].countChildren();
	}
	return 1;
}
ElementFunction::ElementFunction() : MathFunction("element", 2, 3) {
	setArgumentDefinition(1, new VectorArgument(""));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SIZE));
	setDefaultValue(3, "0");
}
int ElementFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[2].number().isPositive() && vargs[0].isMatrix()) {
		size_t row = (size_t) vargs[1].number().uintValue();
		size_t col = (size_t) vargs[2].number().uintValue();
		bool b = true;
		if(col > vargs[0].columns()) {
			CALCULATOR->error(true, _("Column %s does not exist in matrix."), vargs[2].print().c_str(), NULL);
			b = false;
		}
		if(row > vargs[0].rows()) {
			CALCULATOR->error(true, _("Row %s does not exist in matrix."), vargs[1].print().c_str(), NULL);
			b = false;
		}
		if(b) {
			const MathStructure *em = vargs[0].getElement(row, col);
			if(em) mstruct = *em;
			else b = false;
		}
		return b;
	} else {
		if(vargs[2].number().isGreaterThan(1)) {
			CALCULATOR->error(false, _("Argument 3, %s, is ignored for vectors."), getArgumentDefinition(3)->name().c_str(), NULL);
		}
		size_t row = (size_t) vargs[1].number().uintValue();
		if(row > vargs[0].countChildren()) {
			CALCULATOR->error(true, _("Element %s does not exist in vector."), vargs[1].print().c_str(), NULL);
			return 0;
		}
		mstruct = *vargs[0].getChild(row);
		return 1;
	}
}
DimensionFunction::DimensionFunction() : MathFunction("dimension", 1) {
	setArgumentDefinition(1, new VectorArgument(""));
}
int DimensionFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) vargs[0].countChildren();
	return 1;
}
ComponentFunction::ComponentFunction() : MathFunction("component", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(2, new VectorArgument(""));
}
int ComponentFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t i = (size_t) vargs[0].number().uintValue();
	if(i > vargs[1].countChildren()) {
		CALCULATOR->error(true, _("Element %s does not exist in vector."), vargs[0].print().c_str(), NULL);
		return 0;
	}
	mstruct = *vargs[1].getChild(i);
	return 1;
}
LimitsFunction::LimitsFunction() : MathFunction("limits", 3) {
	setArgumentDefinition(1, new VectorArgument(""));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT));
}
int LimitsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].getRange(vargs[1].number().intValue(), vargs[2].number().intValue(), mstruct);
	return 1;
}
AreaFunction::AreaFunction() : MathFunction("area", 5) {
	setArgumentDefinition(1, new MatrixArgument(""));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(4, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(5, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int AreaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].getArea(vargs[1].number().uintValue(), vargs[2].number().uintValue(), vargs[3].number().uintValue(), vargs[4].number().uintValue(), mstruct);
	return 1;
}
TransposeFunction::TransposeFunction() : MathFunction("transpose", 1) {
	setArgumentDefinition(1, new MatrixArgument());
}
int TransposeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	return mstruct.transposeMatrix();
}
IdentityFunction::IdentityFunction() : MathFunction("identity", 1) {
	ArgumentSet *arg = new ArgumentSet();
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	MatrixArgument *marg = new MatrixArgument();
	marg->setSquareDemanded(true);
	arg->addArgument(marg);
	setArgumentDefinition(1, arg);
}
int IdentityFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].isMatrix()) {
		if(vargs[0].rows() != vargs[0].columns()) {
			return 0;
		}
		mstruct.setToIdentityMatrix(vargs[0].size());
	} else {
		mstruct.setToIdentityMatrix((size_t) vargs[0].number().uintValue());
	}
	return 1;
}
DeterminantFunction::DeterminantFunction() : MathFunction("det", 1) {
	MatrixArgument *marg = new MatrixArgument();
	marg->setSquareDemanded(true);
	setArgumentDefinition(1, marg);
}
int DeterminantFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].determinant(mstruct, eo);
	return !mstruct.isUndefined();
}
PermanentFunction::PermanentFunction() : MathFunction("permanent", 1) {
	MatrixArgument *marg = new MatrixArgument();
	marg->setSquareDemanded(true);
	setArgumentDefinition(1, marg);
}
int PermanentFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].permanent(mstruct, eo);
	return !mstruct.isUndefined();
}
CofactorFunction::CofactorFunction() : MathFunction("cofactor", 3) {
	setArgumentDefinition(1, new MatrixArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int CofactorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].cofactor((size_t) vargs[1].number().uintValue(), (size_t) vargs[2].number().uintValue(), mstruct, eo);
	return !mstruct.isUndefined();
}
AdjointFunction::AdjointFunction() : MathFunction("adj", 1) {
	MatrixArgument *marg = new MatrixArgument();
	marg->setSquareDemanded(true);
	setArgumentDefinition(1, marg);
}
int AdjointFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.adjointMatrix(eo);
	return !mstruct.isUndefined();
}
InverseFunction::InverseFunction() : MathFunction("inverse", 1) {
	MatrixArgument *marg = new MatrixArgument();
	marg->setSquareDemanded(true);
	setArgumentDefinition(1, marg);
}
int InverseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	return mstruct.invertMatrix(eo);
}

ZetaFunction::ZetaFunction() : MathFunction("zeta", 1, 1, SIGN_ZETA) {
	IntegerArgument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG);
	setArgumentDefinition(1, arg);
}
int ZetaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {
		mstruct.set(1, 2, 0);
		return 1;
	} else if(vargs[0].number().isMinusOne()) {
		mstruct.set(1, 12, 0);
		return 1;
	} else if(vargs[0].number().isNegative() && vargs[0].number().isEven()) {
		mstruct.clear();
		return 1;
	} else if(vargs[0].number() == 2) {
		mstruct.set(CALCULATOR->v_pi);
		mstruct.raise(2);
		mstruct.divide(6);
		mstruct.mergePrecision(vargs[0]);
		return 1;
	} else if(vargs[0].number() == 4) {
		mstruct.set(CALCULATOR->v_pi);
		mstruct.raise(4);
		mstruct.divide(90);
		mstruct.mergePrecision(vargs[0]);
		return 1;
	} else if(vargs[0].number() == 6) {
		mstruct.set(CALCULATOR->v_pi);
		mstruct.raise(6);
		mstruct.divide(945);
		mstruct.mergePrecision(vargs[0]);
		return 1;
	} else if(vargs[0].number() == 8) {
		mstruct.set(CALCULATOR->v_pi);
		mstruct.raise(8);
		mstruct.divide(9450);
		mstruct.mergePrecision(vargs[0]);
		return 1;
	} else if(vargs[0].number() == 10) {
		mstruct.set(CALCULATOR->v_pi);
		mstruct.raise(10);
		mstruct.divide(93555);
		mstruct.mergePrecision(vargs[0]);
		return 1;
	}
	FR_FUNCTION(zeta)
}
GammaFunction::GammaFunction() : MathFunction("gamma", 1, 1, SIGN_CAPITAL_GAMMA) {
	NON_COMPLEX_NUMBER_ARGUMENT(1);
}
int GammaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isRational() && (eo.approximation == APPROXIMATION_EXACT || (eo.approximation == APPROXIMATION_TRY_EXACT && vargs[0].number().isLessThan(1000)))) {
		if(vargs[0].number().isInteger() && vargs[0].number().isPositive()) {
			mstruct.set(CALCULATOR->f_factorial, &vargs[0], NULL);
			mstruct[0] -= 1;
			return 1;
		} else if(vargs[0].number().denominatorIsTwo()) {
			Number nr(vargs[0].number());
			nr.floor();
			if(nr.isZero()) {
				MathStructure mtmp(CALCULATOR->v_pi);
				mstruct.set(CALCULATOR->f_sqrt, &mtmp, NULL);
				return 1;
			} else if(nr.isPositive()) {
				Number nr2(nr);
				nr2 *= 2;
				nr2 -= 1;
				nr2.doubleFactorial();
				Number nr3(2, 1, 0);
				nr3 ^= nr;
				nr2 /= nr3;
				mstruct = nr2;
				MathStructure mtmp1(CALCULATOR->v_pi);
				MathStructure mtmp2(CALCULATOR->f_sqrt, &mtmp1, NULL);
				mstruct *= mtmp2;
				return 1;
			} else {
				nr.negate();
				Number nr2(nr);
				nr2 *= 2;
				nr2 -= 1;
				nr2.doubleFactorial();
				Number nr3(2, 1, 0);
				nr3 ^= nr;
				if(nr.isOdd()) nr3.negate();
				nr3 /= nr2;
				mstruct = nr3;
				MathStructure mtmp1(CALCULATOR->v_pi);
				MathStructure mtmp2(CALCULATOR->f_sqrt, &mtmp1, NULL);
				mstruct *= mtmp2;
				return 1;
			}
		}
	}
	FR_FUNCTION(gamma)
}
DigammaFunction::DigammaFunction() : MathFunction("digamma", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT(1);
}
int DigammaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isOne()) {
		mstruct.set(CALCULATOR->v_euler);
		mstruct.negate();
		return 1;
	}
	FR_FUNCTION(digamma)
}
BetaFunction::BetaFunction() : MathFunction("beta", 2, 2, SIGN_CAPITAL_BETA) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
}
int BetaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0]; 
	mstruct.set(CALCULATOR->f_gamma, &vargs[0], NULL);
	MathStructure mstruct2(CALCULATOR->f_gamma, &vargs[1], NULL);
	mstruct *= mstruct2;
	mstruct2[0] += vargs[0];
	mstruct /= mstruct2;
	return 1;
}
AiryFunction::AiryFunction() : MathFunction("airy", 1) {
	NumberArgument *arg = new NumberArgument();
	Number fr(-500, 1, 0);
	arg->setMin(&fr);
	fr.set(500, 1, 0);
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
int AiryFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(airy)
}
BesseljFunction::BesseljFunction() : MathFunction("besselj", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	NON_COMPLEX_NUMBER_ARGUMENT(2);
}
int BesseljFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2R(besselj)
}
BesselyFunction::BesselyFunction() : MathFunction("bessely", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	NON_COMPLEX_NUMBER_ARGUMENT(2);
}
int BesselyFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2R(bessely)
}
ErfFunction::ErfFunction() : MathFunction("erf", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT(1);
}
int ErfFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(erf)
}
ErfcFunction::ErfcFunction() : MathFunction("erfc", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT(1);
}
int ErfcFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(erfc)
}

FactorialFunction::FactorialFunction() : MathFunction("factorial", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE, true, false, INTEGER_TYPE_SLONG));
}
int FactorialFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(factorial)
}
bool FactorialFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool FactorialFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool FactorialFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool FactorialFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool FactorialFunction::representsEven(const MathStructure&, bool) const {return false;}
bool FactorialFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool FactorialFunction::representsUndefined(const MathStructure&) const {return false;}

DoubleFactorialFunction::DoubleFactorialFunction() : MathFunction("factorial2", 1) {
	IntegerArgument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG);
	Number nr(-1, 1, 0);
	arg->setMin(&nr);
	setArgumentDefinition(1, arg);
}
int DoubleFactorialFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(doubleFactorial)
}
bool DoubleFactorialFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool DoubleFactorialFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool DoubleFactorialFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool DoubleFactorialFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool DoubleFactorialFunction::representsEven(const MathStructure&, bool) const {return false;}
bool DoubleFactorialFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool DoubleFactorialFunction::representsUndefined(const MathStructure&) const {return false;}

MultiFactorialFunction::MultiFactorialFunction() : MathFunction("multifactorial", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE, true, true, INTEGER_TYPE_SLONG));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SLONG));
}
int MultiFactorialFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(multiFactorial)
}
bool MultiFactorialFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool MultiFactorialFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool MultiFactorialFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool MultiFactorialFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsInteger() && vargs[0].representsNonNegative();}
bool MultiFactorialFunction::representsEven(const MathStructure&, bool) const {return false;}
bool MultiFactorialFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool MultiFactorialFunction::representsUndefined(const MathStructure&) const {return false;}

BinomialFunction::BinomialFunction() : MathFunction("binomial", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true));
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
}
int BinomialFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	Number nr;
	if(!nr.binomial(vargs[0].number(), vargs[1].number())) return 0;
	mstruct = nr;
	return 1;
}

BitXorFunction::BitXorFunction() : MathFunction("bitxor", 2) {
	ArgumentSet *arg = new ArgumentSet();
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONE));
	arg->addArgument(new VectorArgument);
	setArgumentDefinition(1, arg);
	arg = new ArgumentSet();
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONE));
	arg->addArgument(new VectorArgument);
	setArgumentDefinition(2, arg);
}
int BitXorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.add(vargs[1], OPERATION_BITWISE_XOR);
	if(vargs[0].isNumber() && vargs[1].isNumber()) {
		Number nr(vargs[0].number());
		if(nr.bitXor(vargs[1].number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || vargs[0].number().isApproximate() || vargs[1].number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || vargs[0].number().isComplex() || vargs[1].number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || vargs[0].number().includesInfinity() || vargs[1].number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
		return 0;
	} else if(vargs[0].isVector() && vargs[1].isVector()) {
		int i1 = 0, i2 = 1;
		if(vargs[0].size() < vargs[1].size()) {
			i1 = 1;
			i2 = 0;
		}
		mstruct.clearVector();
		mstruct.resizeVector(vargs[i1].size(), m_undefined);
		size_t i = 0;
		for(; i < vargs[i2].size(); i++) {
			mstruct[i].set(CALCULATOR->f_xor, &vargs[i1][i], &vargs[i2][0], NULL);
		}
		for(; i < vargs[i1].size(); i++) {
			mstruct[i] = vargs[i1][i];
			mstruct[i].add(m_zero, OPERATION_GREATER);
		}
		return 1;
	}
	return 0;
}
XorFunction::XorFunction() : MathFunction("xor", 2) {
}
int XorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	int b0, b1;
	if(vargs[0].representsNonPositive(true)) {
		b0 = 0;
	} else if(vargs[0].representsPositive(true)) {
		b0 = 1;
	} else {
		b0 = -1;
	}
	if(vargs[1].representsNonPositive(true)) {
		b1 = 0;
	} else if(vargs[1].representsPositive(true)) {
		b1 = 1;
	} else {
		b1 = -1;
	}
	if((b0 == 1 && b1 == 0) || (b0 == 0 && b1 == 1)) {
		mstruct = m_one;
		return 1;
	} else if(b0 >= 0 && b1 >= 0) {
		return 1;
	} else if(b0 == 0) {
		mstruct = vargs[1];
		mstruct.add(m_zero, OPERATION_GREATER);
		return 1;
	} else if(b0 == 1) {
		mstruct = vargs[1];
		mstruct.add(m_zero, OPERATION_EQUALS_LESS);
		return 1;
	} else if(b1 == 0) {
		mstruct = vargs[0];
		mstruct.add(m_zero, OPERATION_GREATER);
		return 1;
	} else if(b1 == 1) {
		mstruct = vargs[0];
		mstruct.add(m_zero, OPERATION_EQUALS_LESS);
		return 1;
	}
	mstruct = vargs[1];
	mstruct.setLogicalNot();
	mstruct.add(vargs[0], OPERATION_LOGICAL_AND);
	MathStructure mstruct2(vargs[0]);
	mstruct2.setLogicalNot();
	mstruct2.add(vargs[1], OPERATION_LOGICAL_AND);
	mstruct.add(mstruct2, OPERATION_LOGICAL_OR);
	return 1;
}

OddFunction::OddFunction() : MathFunction("odd", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
int OddFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].representsOdd()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsEven()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.representsOdd()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(mstruct.representsEven()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}
EvenFunction::EvenFunction() : MathFunction("even", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
int EvenFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsOdd()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(mstruct.representsOdd()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}
ShiftFunction::ShiftFunction() : MathFunction("shift", 2) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}
int ShiftFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(shift)
}

AbsFunction::AbsFunction() : MathFunction("abs", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool AbsFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsInteger(allow_units);}
bool AbsFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsRational(allow_units);}
bool AbsFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsEven(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsEven(allow_units);}
bool AbsFunction::representsOdd(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsOdd(allow_units);}
bool AbsFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int AbsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0]; 
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(!nr.abs() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			return -1;
		}
		mstruct = nr; 
		return 1;
	}
	if(mstruct.representsNegative(true)) {
		mstruct.negate();
		return 1;
	}
	if(mstruct.representsNonNegative(true)) {
		return 1;
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].transform(STRUCT_FUNCTION);
			mstruct[i].setFunction(this);
		}
		mstruct.childrenUpdated();
		return 1;
	}
	if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 1) {
		mstruct.setFunction(this);
		mstruct.transform(STRUCT_FUNCTION);
		mstruct.setFunction(CALCULATOR->f_signum);
		return 1;
	}
	return -1;
}
GcdFunction::GcdFunction() : MathFunction("gcd", 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	RATIONAL_POLYNOMIAL_ARGUMENT(2)
}
int GcdFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(MathStructure::gcd(vargs[0], vargs[1], mstruct, eo)) {	
		return 1;
	}
	return 0;
}
LcmFunction::LcmFunction() : MathFunction("lcm", 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	RATIONAL_POLYNOMIAL_ARGUMENT(2)
}
int LcmFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(MathStructure::lcm(vargs[0], vargs[1], mstruct, eo)) {
		return 1;
	}
	return 0;
}
SignumFunction::SignumFunction() : MathFunction("sgn", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool SignumFunction::representsPositive(const MathStructure&, bool allow_units) const {return false;}
bool SignumFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative(true);}
bool SignumFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive(true);}
bool SignumFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool SignumFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex(true);}
bool SignumFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero(true);}
bool SignumFunction::representsEven(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero(true);}
bool SignumFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int SignumFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		Number nr(mstruct.number()); 
		if(!nr.signum() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			if(mstruct.number().isNonZero()) {
				MathStructure *mabs = new MathStructure(mstruct);
				mabs->transform(STRUCT_FUNCTION);
				mabs->setFunction(CALCULATOR->f_abs);
				mstruct.divide_nocopy(mabs);
				return 1;
			}
			return -1;
		} else {
			mstruct = nr; 
			return 1;
		}
	}
	if(mstruct.representsPositive(true)) {
		mstruct.set(1, 1, 0);
		return 1;
	}
	if(mstruct.representsNegative(true)) {
		mstruct.set(-1, 1, 0);
		return 1;
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].transform(STRUCT_FUNCTION);
			mstruct[i].setFunction(this);
		}
		mstruct.childrenUpdated();
		return 1;
	}
	/*if(mstruct.representsNonZero(true)) {
		MathStructure *mabs = new MathStructure(mstruct);
		mabs->transform(STRUCT_FUNCTION);
		mabs->setFunction(CALCULATOR->f_abs);
		mstruct.divide_nocopy(mabs);
		return 1;
	}*/
	return -1;
}

CeilFunction::CeilFunction() : MathFunction("ceil", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int CeilFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(ceil)
}
bool CeilFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool CeilFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool CeilFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool CeilFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool CeilFunction::representsUndefined(const MathStructure&) const {return false;}

FloorFunction::FloorFunction() : MathFunction("floor", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int FloorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(floor)
}
bool FloorFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool FloorFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool FloorFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool FloorFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool FloorFunction::representsUndefined(const MathStructure&) const {return false;}

TruncFunction::TruncFunction() : MathFunction("trunc", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int TruncFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(trunc)
}
bool TruncFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool TruncFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool TruncFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool TruncFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool TruncFunction::representsUndefined(const MathStructure&) const {return false;}

RoundFunction::RoundFunction() : MathFunction("round", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int RoundFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(round)
}
bool RoundFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool RoundFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool RoundFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool RoundFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool RoundFunction::representsUndefined(const MathStructure&) const {return false;}

FracFunction::FracFunction() : MathFunction("frac", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int FracFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(frac)
}
IntFunction::IntFunction() : MathFunction("int", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int IntFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(trunc)
}
NumeratorFunction::NumeratorFunction() : MathFunction("numerator", 1) {
	NumberArgument *arg_rational_1 = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false); 
	arg_rational_1->setRationalNumber(true); 
	setArgumentDefinition(1, arg_rational_1);
}
int NumeratorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isNumber()) {
		if(vargs[0].number().isInteger()) {
			mstruct = vargs[0];
			return 1;
		} else if(vargs[0].number().isRational()) {
			mstruct.set(vargs[0].number().numerator());
			return 1;
		}
		return 0;
	} else if(vargs[0].representsInteger()) {
		mstruct = vargs[0];
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.representsInteger()) {
		return 1;
	} else if(mstruct.isNumber() && mstruct.number().isRational()) {
		Number nr(mstruct.number().numerator());
		mstruct.set(nr);
		return 1;
	}
	return -1;
}
DenominatorFunction::DenominatorFunction() : MathFunction("denominator", 1) {
	RATIONAL_NUMBER_ARGUMENT_NO_ERROR(1)
}
int DenominatorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.set(vargs[0].number().denominator());
	return 1;
}
RemFunction::RemFunction() : MathFunction("rem", 2) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
int RemFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(rem)
}
ModFunction::ModFunction() : MathFunction("mod", 2) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
int ModFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(mod)
}

PolynomialUnitFunction::PolynomialUnitFunction() : MathFunction("punit", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int PolynomialUnitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.set(vargs[0].polynomialUnit(vargs[1]), 0);
	return 1;
}
PolynomialPrimpartFunction::PolynomialPrimpartFunction() : MathFunction("primpart", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int PolynomialPrimpartFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialPrimpart(vargs[1], mstruct, eo);
	return 1;
}
PolynomialContentFunction::PolynomialContentFunction() : MathFunction("pcontent", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int PolynomialContentFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialContent(vargs[1], mstruct, eo);
	return 1;
}
CoeffFunction::CoeffFunction() : MathFunction("coeff", 2, 3) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "x");
}
int CoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].coefficient(vargs[2], vargs[1].number(), mstruct);
	return 1;
}
LCoeffFunction::LCoeffFunction() : MathFunction("lcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int LCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].lcoefficient(vargs[1], mstruct);
	return 1;
}
TCoeffFunction::TCoeffFunction() : MathFunction("tcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int TCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].tcoefficient(vargs[1], mstruct);
	return 1;
}
DegreeFunction::DegreeFunction() : MathFunction("degree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int DegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].degree(vargs[1]);
	return 1;
}
LDegreeFunction::LDegreeFunction() : MathFunction("ldegree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
}
int LDegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].ldegree(vargs[1]);
	return 1;
}


ImFunction::ImFunction() : MathFunction("im", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
int ImFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().imaginaryPart();
		return 1;
	} else if(mstruct.representsReal()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}
bool ImFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ImFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ImFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ImFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ImFunction::representsUndefined(const MathStructure&) const {return false;}

ReFunction::ReFunction() : MathFunction("re", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
int ReFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().realPart();
		return 1;
	} else if(mstruct.representsReal()) {
		return 1;
	}
	return -1;
}
bool ReFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool ReFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNegative();}
bool ReFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool ReFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive();}
bool ReFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger();}
bool ReFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ReFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsRational();}
bool ReFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ReFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ReFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonZero();}
bool ReFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsEven();}
bool ReFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsOdd();}
bool ReFunction::representsUndefined(const MathStructure&) const {return false;}

SqrtFunction::SqrtFunction() : MathFunction("sqrt", 1) {
}
int SqrtFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.raise(nr_half);
	return 1;
}
bool SqrtFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsPositive(allow_units);}
bool SqrtFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool SqrtFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool SqrtFunction::representsRational(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool SqrtFunction::representsComplex(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNegative(allow_units);}
bool SqrtFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonZero(allow_units);}
bool SqrtFunction::representsEven(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsUndefined(const MathStructure&) const {return false;}
CbrtFunction::CbrtFunction() : MathFunction("cbrt", 1) {
}
int CbrtFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].representsNegative(true)) {
		mstruct = vargs[0];
		mstruct.negate();
		mstruct.raise(Number(1, 3, 0));
		mstruct.negate();
	} else if(vargs[0].representsNonNegative(true)) {
		mstruct = vargs[0];
		mstruct.raise(Number(1, 3, 0));
	} else {
		MathStructure mroot(3, 1, 0);
		mstruct.set(CALCULATOR->f_root, &vargs[0], &mroot, NULL);
	}
	return 1;
}
RootFunction::RootFunction() : MathFunction("root", 2) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_POSITIVE, false, false);
	arg->setComplexAllowed(false);
	setArgumentDefinition(1, arg);
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true));
}
int RootFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].number().isOne()) {
		mstruct = vargs[0];
		return 1;
	}
	if(vargs[0].representsNonNegative(true)) {
		mstruct = vargs[0];
		Number nr_exp(vargs[1].number());
		nr_exp.recip();
		mstruct.raise(nr_exp);
		return 1;
	} else if(vargs[1].number().isOdd() && vargs[0].representsNegative(true)) {
		mstruct = vargs[0];
		mstruct.negate();
		Number nr_exp(vargs[1].number());
		nr_exp.recip();
		mstruct.raise(nr_exp);
		mstruct.negate();
		return 1;
	}
	bool eval_mstruct = !vargs[0].isNumber();
	Number nr;
	if(eval_mstruct) {
		mstruct = vargs[0];
		if(eo.approximation == APPROXIMATION_TRY_EXACT) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			mstruct.eval(eo2);
		} else {
			mstruct.eval(eo);
		}
		if(mstruct.representsNonNegative(true)) {
			Number nr_exp(vargs[1].number());
			nr_exp.recip();
			mstruct.raise(nr_exp);
			return 1;
		} else if(vargs[1].number().isOdd() && mstruct.representsNegative(true)) {
			mstruct.negate();
			Number nr_exp(vargs[1].number());
			nr_exp.recip();
			mstruct.raise(nr_exp);
			mstruct.negate();
			return 1;
		}
		if(!mstruct.isNumber()) {
			if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isInteger()) {
				if(mstruct[1] == vargs[1]) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct.delChild(2);
						mstruct.setType(STRUCT_FUNCTION);
						mstruct.setFunction(CALCULATOR->f_abs);
					} else {
						mstruct.setToChild(1);
					}
					return 1;
				} else if(mstruct[1].number().isIntegerDivisible(vargs[1].number())) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
					}
					mstruct[1].divide(vargs[1].number());
					return 1;
				} else if(!mstruct[1].number().isMinusOne() && vargs[1].number().isIntegerDivisible(mstruct[1].number())) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
					}
					Number new_root(vargs[1].number());
					new_root.divide(mstruct[1].number());
					bool bdiv = new_root.isNegative();
					if(bdiv) new_root.negate();
					mstruct[1] = new_root;
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
					if(bdiv) mstruct.raise(nr_minus_one);
					return 1;
				} else if(mstruct[1].number().isMinusOne()) {
					mstruct[0].transform(STRUCT_FUNCTION, vargs[1]);
					mstruct[0].setFunction(this);
					return 1;
				} else if(mstruct[1].number().isNegative()) {
					mstruct[1].number().negate();
					mstruct.transform(STRUCT_FUNCTION, vargs[1]);
					mstruct.setFunction(this);
					mstruct.raise(nr_minus_one);
					return 1;
				}
			} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsTwo()) {
				Number new_root(vargs[1].number());
				new_root.multiply(Number(2, 1, 0));
				if(mstruct[1].number().numeratorIsOne()) {
					mstruct[1].number().set(new_root, true);
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
				} else {
					mstruct[1].number().set(mstruct[1].number().numerator(), true);
					mstruct.transform(STRUCT_FUNCTION);
					mstruct.addChild(new_root);
					mstruct.setFunction(this);
				}
				return 1;
			} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_sqrt && mstruct.size() == 1) {
				Number new_root(vargs[1].number());
				new_root.multiply(Number(2, 1, 0));
				mstruct.addChild(new_root);
				mstruct.setFunction(this);
				return 1;
			} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_root && mstruct.size() == 2 && mstruct[1].isNumber() && mstruct[1].number().isInteger() && mstruct[1].number().isPositive()) {
				Number new_root(vargs[1].number());
				new_root.multiply(mstruct[1].number());
				mstruct[1] = new_root;
				return 1;
			} else if(mstruct.isMultiplication()) {
				bool b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					if(!mstruct[i].representsReal(true)) {
						b = false;
						break;
					}
				}
				if(b) {
					bool b_neg = false;
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(mstruct[i].isNumber() && mstruct[i].number().isNegative() && !mstruct[i].isMinusOne()) {
							mstruct[i].negate();
							b_neg = !b_neg;
						}
						mstruct[i].transform(STRUCT_FUNCTION, vargs[1]);
						mstruct[i].setFunction(this);
					}
					if(b_neg) mstruct.insertChild_nocopy(new MathStructure(-1, 1, 0), 1);
					return 1;
				}
			}
			return -1;
		}
		nr = mstruct.number();
	} else {
		nr = vargs[0].number(); 
	}
	if(!nr.root(vargs[1].number()) || (eo.approximation < APPROXIMATION_APPROXIMATE && nr.isApproximate() && !vargs[0].isApproximate() && !mstruct.isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		if(!eval_mstruct) {
			if(vargs[0].number().isNegative() && vargs[1].number().isOdd()) {
				mstruct.set(this, &vargs[0], &vargs[1], NULL);
				mstruct[0].number().negate();
				mstruct.negate();
				return 1;
			}
			return 0;
		} else if(mstruct.number().isNegative() && vargs[1].number().isOdd()) {
			mstruct.number().negate();
			mstruct.transform(STRUCT_FUNCTION, vargs[1]);
			mstruct.setFunction(this);
			mstruct.negate();
			return 1;
		}
	} else {
		mstruct.set(nr); 
		return 1;
	}
	return -1;
}
bool RootFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsPositive(allow_units);}
bool RootFunction::representsNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsOdd() && vargs[1].representsPositive() && vargs[0].representsNegative(allow_units);}
bool RootFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNonNegative(allow_units);}
bool RootFunction::representsNonPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsOdd() && vargs[1].representsPositive() && vargs[0].representsNonPositive(allow_units);}
bool RootFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool RootFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNumber(allow_units);}
bool RootFunction::representsRational(const MathStructure&, bool) const {return false;}
bool RootFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsReal(allow_units) && (vargs[0].representsNonNegative(allow_units) || vargs[1].representsOdd());}
bool RootFunction::representsComplex(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && (vargs[0].representsComplex(allow_units) || (vargs[1].representsEven() && vargs[0].representsNegative(allow_units)));}
bool RootFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNonZero(allow_units);}
bool RootFunction::representsEven(const MathStructure&, bool) const {return false;}
bool RootFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool RootFunction::representsUndefined(const MathStructure&) const {return false;}

SquareFunction::SquareFunction() : MathFunction("sq", 1) {
}
int SquareFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	mstruct ^= 2;
	return 1;
}

ExpFunction::ExpFunction() : MathFunction("exp", 1) {
}
int ExpFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = CALCULATOR->v_e;
	mstruct ^= vargs[0];
	return 1;
}

LogFunction::LogFunction() : MathFunction("ln", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false));
}
bool LogFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive() && ((vargs[0].isNumber() && vargs[0].number().isGreaterThan(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && !((KnownVariable*) vargs[0].variable())->get().number().isGreaterThan(nr_one)));}
bool LogFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative() && ((vargs[0].isNumber() && vargs[0].number().isLessThan(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && !((KnownVariable*) vargs[0].variable())->get().number().isLessThan(nr_one)));}
bool LogFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive() && ((vargs[0].isNumber() && vargs[0].number().isGreaterThanOrEqualTo(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && !((KnownVariable*) vargs[0].variable())->get().number().isGreaterThanOrEqualTo(nr_one)));}
bool LogFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative() && ((vargs[0].isNumber() && vargs[0].number().isLessThanOrEqualTo(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && !((KnownVariable*) vargs[0].variable())->get().number().isLessThanOrEqualTo(nr_one)));}
bool LogFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool LogFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber() && vargs[0].representsNonZero();}
bool LogFunction::representsRational(const MathStructure&, bool) const {return false;}
bool LogFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool LogFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool LogFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsNonPositive() || (vargs[0].isNumber() && !vargs[0].number().isOne()) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && !((KnownVariable*) vargs[0].variable())->get().number().isOne()));}
bool LogFunction::representsEven(const MathStructure&, bool) const {return false;}
bool LogFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool LogFunction::representsUndefined(const MathStructure&) const {return false;}
int LogFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[0]; 

	int errors = 0;	
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages(&errors);
	} else {
		mstruct.eval(eo);
	}
	bool b = false;
	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_e) {
		mstruct.set(m_one);
		b = true;
	} else if(mstruct.isPower()) {
		if(mstruct[0].isVariable() && mstruct[0].variable() == CALCULATOR->v_e) {
			if(mstruct[1].representsReal()) {
				mstruct.setToChild(2, true);
				b = true;
			}
		} else if(mstruct[1].representsPositive() || (mstruct[1].representsNegative() && mstruct[0].representsPositive())) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			b = true;
		}
	} else if(mstruct.isMultiplication()) {
		b = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsPositive()) {
				b = false;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			for(size_t i = 1; i < mstruct.size(); i++) {
				mstruct2.add(MathStructure(CALCULATOR->f_ln, &mstruct[i], NULL), i > 1);
			}
			mstruct = mstruct2;
		}
	}
	if(b) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT && errors > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			MathStructure mstruct2 = vargs[0];
			mstruct2.eval(eo2);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		mstruct.eval(eo2);
	}
	if(mstruct.isNumber()) {
		if(eo.allow_complex && mstruct.number().isMinusOne()) {
			mstruct = CALCULATOR->v_i->get();
			mstruct *= CALCULATOR->v_pi;
			return 1;
		} else if(mstruct.number().isI() || mstruct.number().isMinusI()) {
			mstruct.set(1, 2, 0);
			mstruct *= CALCULATOR->v_pi;
			mstruct *= CALCULATOR->v_i->get();
			return 1;
		} else if(eo.allow_complex && eo.allow_infinite && mstruct.number().isMinusInfinity()) {
			mstruct = CALCULATOR->v_pi;
			mstruct *= CALCULATOR->v_i->get();
			Number nr; nr.setPlusInfinity();
			mstruct += nr;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.ln() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	return -1;
	
}
LognFunction::LognFunction() : MathFunction("log", 1, 2) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false));
	setDefaultValue(2, "e");
}
int LognFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[1].isVariable() && vargs[1].variable() == CALCULATOR->v_e) {
		mstruct.set(CALCULATOR->f_ln, &vargs[0], NULL);
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	MathStructure mstructv2 = vargs[1];
	mstructv2.eval(eo);
	if(mstruct.isPower()) {
		if(mstruct[1].representsPositive() || (mstruct[1].representsNegative() && mstruct[0].representsPositive())) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_logn, &mstruct[0], &mstructv2, NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			return 1;
		}
	} else if(mstruct.isMultiplication()) {
		bool b = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsPositive()) {
				b = false;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_logn, &mstruct[0], &mstructv2, NULL);
			for(size_t i = 1; i < mstruct.size(); i++) {
				mstruct2.add(MathStructure(CALCULATOR->f_logn, &mstruct[i], &mstructv2, NULL), i > 1);
			}
			mstruct = mstruct2;
			return 1;
		}
	} else if(mstruct.isNumber() && mstructv2.isNumber()) {
		Number nr(mstruct.number());
		if(nr.log(mstructv2.number()) && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	mstruct.set(CALCULATOR->f_ln, &vargs[0], NULL);
	mstruct.divide_nocopy(new MathStructure(CALCULATOR->f_ln, &vargs[1], NULL));
	return 1;
}

LambertWFunction::LambertWFunction() : MathFunction("lambertw", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false);
	arg->setComplexAllowed(false);	
	setArgumentDefinition(1, arg);
}
int LambertWFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[0]; 

	int errors = 0;	
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages(&errors);
	} else {
		mstruct.eval(eo);
	}
	bool b = false;
	if(mstruct.isZero()) {
		b = true;
	} else if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_e) {
		mstruct.set(m_one);
		b = true;
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne() && mstruct[1].isPower() && mstruct[1][0].isVariable() && mstruct[1][0].variable() == CALCULATOR->v_e && mstruct[1][1].isMinusOne()) {
		mstruct = -1;
		b = true;		
	}
	if(b) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT && errors > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			MathStructure mstruct2 = vargs[0];
			mstruct2.eval(eo2);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT) return -1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		mstruct.eval(eo2);
	}
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(!nr.lambertW()) {
			if(!CALCULATOR->aborted()) CALCULATOR->error(false, _("Argument for %s() must be a real number greater than or equal to -1/e."), preferredDisplayName().name.c_str(), NULL);
		} else if(!(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	return -1;
}

bool is_real_angle_value(const MathStructure &mstruct) {
	if(mstruct.isUnit()) {
		return mstruct.unit() == CALCULATOR->getRadUnit() || mstruct.unit() == CALCULATOR->getDegUnit() || mstruct.unit() == CALCULATOR->getGraUnit();
	} else if(mstruct.isMultiplication()) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b && mstruct[i].isUnit()) {
				if(mstruct[i].unit() == CALCULATOR->getRadUnit() || mstruct[i].unit() == CALCULATOR->getDegUnit() || mstruct[i].unit() == CALCULATOR->getGraUnit()) {
					b = true;
				} else {
					return false;
				}
			} else if(!mstruct[i].representsReal()) {
				return false;
			}
		}
		return b;
	}
	return false;
}
bool is_number_angle_value(const MathStructure &mstruct) {
	if(mstruct.isUnit()) {
		return mstruct.unit() == CALCULATOR->getRadUnit() || mstruct.unit() == CALCULATOR->getDegUnit() || mstruct.unit() == CALCULATOR->getGraUnit();
	} else if(mstruct.isMultiplication()) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b && mstruct[i].isUnit()) {
				if(mstruct[i].unit() == CALCULATOR->getRadUnit() || mstruct[i].unit() == CALCULATOR->getDegUnit() || mstruct[i].unit() == CALCULATOR->getGraUnit()) {
					b = true;
				} else {
					return false;
				}
			} else if(!mstruct[i].representsNumber()) {
				return false;
			}
		}
		return b;
	}
	return false;
}

SinFunction::SinFunction() : MathFunction("sin", 1) {
	setArgumentDefinition(1, new AngleArgument());
}
bool SinFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_number_angle_value(vargs[0]);}
bool SinFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_real_angle_value(vargs[0]);}
int SinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[0]; 
	if(CALCULATOR->getRadUnit()) {
		mstruct.convert(CALCULATOR->getRadUnit());
		mstruct /= CALCULATOR->getRadUnit();
	}
	int errors = 0;
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages(&errors);
	} else {
		mstruct.eval(eo);
	}
	bool b = false;
	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_pi) {
		mstruct.clear();
		b = true;
	} else if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function() == CALCULATOR->f_asin) {
			MathStructure mstruct_new(mstruct[0]);
			mstruct = mstruct_new;
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
		if(mstruct[0].number().isInteger()) {
			mstruct.clear();
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			Number nr_int(mstruct[0].number());
			nr_int.floor();
			bool b_even = nr_int.isEven();
			nr.setNegative(false);
			if(nr.equals(Number(1, 2, 0))) {
				if(b_even) mstruct.set(1, 1, 0);
				else mstruct.set(-1, 1, 0);
				b = true;
			} else if(nr.equals(Number(1, 4, 0)) || nr.equals(Number(3, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 3, 0)) || nr.equals(Number(2, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 6, 0)) || nr.equals(Number(5, 6, 0))) {
				if(b_even) mstruct.set(1, 2, 0);
				else mstruct.set(-1, 2, 0);
				b = true;
			}
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		bool b_negate = false;
		for(; i < mstruct.size(); i++) {
			if(mstruct[i] == CALCULATOR->v_pi || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1] == CALCULATOR->v_pi && mstruct[i][0].isNumber() && mstruct[i][0].number().isInteger())) {
				b_negate = mstruct[i] == CALCULATOR->v_pi || mstruct[i][0].number().isOdd();
				b = true;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if(i2 != i) {
					if(mstruct2.isZero()) {
						mstruct2 = mstruct[i2];
					} else {
						mstruct2.add(mstruct[i2], true);
					}
				}
			}
			if(CALCULATOR->getRadUnit()) mstruct2 *= CALCULATOR->getRadUnit();
			mstruct.set(CALCULATOR->f_sin, &mstruct2, NULL);
			if(b_negate) mstruct.negate();
		}
	}

	if(b) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT && errors > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			MathStructure mstruct2 = vargs[0];
			if(CALCULATOR->getRadUnit()) {
				mstruct2.convert(CALCULATOR->getRadUnit());
				mstruct2 /= CALCULATOR->getRadUnit();
			}
			mstruct2.eval(eo2);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		if(CALCULATOR->getRadUnit()) {
			mstruct.convert(CALCULATOR->getRadUnit());
			mstruct /= CALCULATOR->getRadUnit();
		}
		mstruct.eval(eo2);
	}

	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.sin() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	if(mstruct.isNegate()) {
		MathStructure mstruct2(CALCULATOR->f_sin, &mstruct[0], NULL);
		mstruct = mstruct2;
		mstruct.negate();
		if(CALCULATOR->getRadUnit()) mstruct[0] *= CALCULATOR->getRadUnit();
		return 1;
	}
	if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();
	return -1;
}

CosFunction::CosFunction() : MathFunction("cos", 1) {
	setArgumentDefinition(1, new AngleArgument());
}
bool CosFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_number_angle_value(vargs[0]);}
bool CosFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_real_angle_value(vargs[0]);}
int CosFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[0]; 
	if(CALCULATOR->getRadUnit()) {
		mstruct.convert(CALCULATOR->getRadUnit());
		mstruct /= CALCULATOR->getRadUnit();
	}
	
	int errors = 0;
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages(&errors);
	} else {
		mstruct.eval(eo);
	}
	bool b = false;
	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_pi) {
		mstruct = -1;
		b = true;
	} else if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function() == CALCULATOR->f_acos) {
			MathStructure mstruct_new(mstruct[0]);
			mstruct = mstruct_new;
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
		if(mstruct[0].number().isInteger()) {
			if(mstruct[0].number().isEven()) {
				mstruct = 1;
			} else {
				mstruct = -1;
			}
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			Number nr_int(mstruct[0].number());
			nr_int.trunc();
			bool b_even = nr_int.isEven();
			nr.setNegative(false);
			if(nr.equals(Number(1, 2, 0))) {
				mstruct.clear();
				b = true;
			} else if(nr.equals(Number(1, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(3, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 3, 0))) {
				if(b_even) mstruct.set(1, 2, 0);
				else mstruct.set(-1, 2, 0);
				b = true;
			} else if(nr.equals(Number(2, 3, 0))) {
				if(b_even) mstruct.set(-1, 2, 0);
				else mstruct.set(1, 2, 0);
				b = true;
			} else if(nr.equals(Number(1, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(5, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(b_even) mstruct.negate();
				b = true;
			}
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		bool b_negate = false;
		for(; i < mstruct.size(); i++) {
			if(mstruct[i] == CALCULATOR->v_pi || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1] == CALCULATOR->v_pi && mstruct[i][0].isNumber() && mstruct[i][0].number().isInteger())) {
				b_negate = mstruct[i] == CALCULATOR->v_pi || mstruct[i][0].number().isOdd();
				b = true;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if(i2 != i) {
					if(mstruct2.isZero()) {
						mstruct2 = mstruct[i2];
					} else {
						mstruct2.add(mstruct[i2], true);
					}
				}
			}
			if(CALCULATOR->getRadUnit()) mstruct2 *= CALCULATOR->getRadUnit();
			mstruct.set(CALCULATOR->f_cos, &mstruct2, NULL);
			if(b_negate) mstruct.negate();
		}
	}
	if(b) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT && errors > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			MathStructure mstruct2 = vargs[0];
			if(CALCULATOR->getRadUnit()) {
				mstruct2.convert(CALCULATOR->getRadUnit());
				mstruct2 /= CALCULATOR->getRadUnit();
			}
			mstruct2.eval(eo2);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		if(CALCULATOR->getRadUnit()) {
			mstruct.convert(CALCULATOR->getRadUnit());
			mstruct /= CALCULATOR->getRadUnit();
		}
		mstruct.eval(eo2);
	}
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.cos() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();
	return -1;
}

TanFunction::TanFunction() : MathFunction("tan", 1) {
	setArgumentDefinition(1, new AngleArgument());
}
bool TanFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_number_angle_value(vargs[0]);}
bool TanFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && is_real_angle_value(vargs[0]);}
int TanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0]; 
	if(CALCULATOR->getRadUnit()) {
		mstruct.convert(CALCULATOR->getRadUnit());
		mstruct /= CALCULATOR->getRadUnit();
	}
	int errors = 0;
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages(&errors);
	} else {
		mstruct.eval(eo);
	}
	bool b = false;
	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_pi) {
		mstruct.clear();
		b = true;
	} else if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function() == CALCULATOR->f_atan) {
			MathStructure mstruct_new(mstruct[0]);
			mstruct = mstruct_new;
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
		if(mstruct[0].number().isInteger()) {
			mstruct.clear();
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			bool b_neg = nr.isNegative();
			nr.setNegative(false);
			if(nr.equals(nr_half)) {
				Number nr_int(mstruct[0].number());
				nr_int.floor();
				bool b_even = nr_int.isEven();
				if(b_even) nr.setPlusInfinity();
				else nr.setMinusInfinity();
				mstruct.set(nr);
				b = true;
			} else if(nr.equals(Number(1, 4, 0))) {
				if(b_neg) mstruct.set(-1, 1, 0);
				else mstruct.set(1, 1, 0);
				b = true;
			} else if(nr.equals(Number(3, 4, 0))) {
				if(!b_neg) mstruct.set(-1, 1, 0);
				else mstruct.set(1, 1, 0);
				b = true;
			} else if(nr.equals(Number(1, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(2, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(!b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(-1, 2, 0));
				if(b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(5, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(-1, 2, 0));
				if(!b_neg) mstruct.negate();
				b = true;
			}
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		for(; i < mstruct.size(); i++) {
			if(mstruct[i] == CALCULATOR->v_pi || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1] == CALCULATOR->v_pi && mstruct[i][0].isNumber() && mstruct[i][0].number().isInteger())) {
				b = true;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if(i2 != i) {
					if(mstruct2.isZero()) {
						mstruct2 = mstruct[i2];
					} else {
						mstruct2.add(mstruct[i2], true);
					}
				}
			}
			if(CALCULATOR->getRadUnit()) mstruct2 *= CALCULATOR->getRadUnit();
			mstruct.set(CALCULATOR->f_tan, &mstruct2, NULL);
		}
	}

	if(b) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT && errors > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			MathStructure mstruct2 = vargs[0];
			if(CALCULATOR->getRadUnit()) {
				mstruct2.convert(CALCULATOR->getRadUnit());
				mstruct2 /= CALCULATOR->getRadUnit();
			}
			mstruct2.eval(eo2);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		if(CALCULATOR->getRadUnit()) {
			mstruct.convert(CALCULATOR->getRadUnit());
			mstruct /= CALCULATOR->getRadUnit();
		}
		mstruct.eval(eo2);
	}

	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.tan() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	if(mstruct.isNegate()) {
		MathStructure mstruct2(CALCULATOR->f_tan, &mstruct[0], NULL);
		mstruct = mstruct2;
		mstruct.negate();
		if(CALCULATOR->getRadUnit()) mstruct[0] *= CALCULATOR->getRadUnit();
		return 1;
	}
	if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();
	return -1;
}

AsinFunction::AsinFunction() : MathFunction("asin", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AsinFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
int AsinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	
	if(vargs[0].number().isZero()) {
		mstruct.clear();
	} else if(vargs[0].number().isOne()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(90, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(100, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().isMinusOne()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(-90, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(-100, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(-1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(-1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().equals(nr_half)) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(30, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(100, 3, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 6, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 6, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else {
		Number nr = vargs[0].number();
		if(!nr.asin() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) return 0;
		mstruct = nr;
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.multiply_nocopy(new MathStructure(180, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.multiply_nocopy(new MathStructure(200, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				break;
			}
			default: {
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	}
	return 1;
	
}

AcosFunction::AcosFunction() : MathFunction("acos", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AcosFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
int AcosFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	
	if(vargs[0].number().isZero()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(90, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(100, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().isOne()) {
		mstruct.clear();
	} else if(vargs[0].number().isMinusOne()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(180, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(200, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(CALCULATOR->v_pi);
				break;
			}
			default: {
				mstruct.set(CALCULATOR->v_pi);
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().equals(nr_half)) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(60, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(200, 3, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 3, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 3, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else {
		Number nr = vargs[0].number();
		if(!nr.acos() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) return 0;
		mstruct = nr;
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.multiply_nocopy(new MathStructure(180, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.multiply_nocopy(new MathStructure(200, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				break;
			}
			default: {
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	}
	return 1;
	
}

AtanFunction::AtanFunction() : MathFunction("atan", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AtanFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber() && !vargs[0].number().isI() && !vargs[0].number().isMinusI();}
bool AtanFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
int AtanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	
	if(vargs[0].number().isZero()) {
		mstruct.clear();
	} else if(eo.allow_infinite && vargs[0].number().isI()) {
		mstruct = vargs[0];
		Number nr; nr.setImaginaryPart(nr_plus_inf);
		mstruct *= nr;
	} else if(eo.allow_infinite && vargs[0].number().isMinusI()) {
		mstruct = vargs[0];
		Number nr; nr.setImaginaryPart(nr_minus_inf);
		mstruct *= nr;
	} else if(vargs[0].number().isPlusInfinity()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(90, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(100, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().isMinusInfinity()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(-90, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(-100, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(-1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(-1, 2, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().isOne()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(45, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(50, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(1, 4, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(1, 4, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else if(vargs[0].number().isMinusOne()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.set(-45, 1, 0);
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.set(-50, 1, 0);
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				mstruct.set(-1, 4, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			default: {
				mstruct.set(-1, 4, 0);
				mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi));
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	} else {
		Number nr = vargs[0].number();
		if(!nr.atan() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) return 0;
		mstruct = nr;
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {
				mstruct.multiply_nocopy(new MathStructure(180, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_GRADIANS: {
				mstruct.multiply_nocopy(new MathStructure(200, 1, 0));
				mstruct.divide_nocopy(new MathStructure(CALCULATOR->v_pi));
				break;
			}
			case ANGLE_UNIT_RADIANS: {
				break;
			}
			default: {
				if(CALCULATOR->getRadUnit()) {
					mstruct *= CALCULATOR->getRadUnit();
				}
			}
		}
	}
	return 1;
	
}

SinhFunction::SinhFunction() : MathFunction("sinh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool SinhFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool SinhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
int SinhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(sinh)
}
CoshFunction::CoshFunction() : MathFunction("cosh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool CoshFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool CoshFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
int CoshFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(cosh)
}
TanhFunction::TanhFunction() : MathFunction("tanh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool TanhFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool TanhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
int TanhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&eo) {
	FR_FUNCTION(tanh)
}
AsinhFunction::AsinhFunction() : MathFunction("asinh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AsinhFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool AsinhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
int AsinhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(asinh)
}
AcoshFunction::AcoshFunction() : MathFunction("acosh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AcoshFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
int AcoshFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(acosh)
}
AtanhFunction::AtanhFunction() : MathFunction("atanh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
int AtanhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(atanh)
}
Atan2Function::Atan2Function() : MathFunction("atan2", 2) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg->setComplexAllowed(false);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);	
	arg->setComplexAllowed(false);
	setArgumentDefinition(2, arg);
}
bool Atan2Function::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[0].representsNumber() && vargs[1].representsNumber();}
int Atan2Function::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {
		if(!vargs[1].number().isNonZero()) return 0;
		if(vargs[1].number().isNegative()) {
			switch(eo.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {mstruct.set(180, 1, 0); break;}
				case ANGLE_UNIT_GRADIANS: {mstruct.set(200, 1, 0); break;}
				case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->v_pi); break;}
				default: {mstruct.set(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
			}
		} else {
			mstruct.clear();
		}
	} else if(vargs[1].number().isZero() && vargs[0].number().isNonZero()) {
		switch(eo.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {mstruct.set(90, 1, 0); break;}
			case ANGLE_UNIT_GRADIANS: {mstruct.set(100, 1, 0); break;}
			case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->v_pi); mstruct.multiply(nr_half); break;}
			default: {mstruct.set(CALCULATOR->v_pi); mstruct.multiply(nr_half); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
		}
		if(vargs[0].number().hasNegativeSign()) mstruct.negate();
	} else if(!vargs[1].number().isNonZero()) {
		FR_FUNCTION_2(atan2)
	} else {
		MathStructure new_nr(vargs[0]);
		if(!new_nr.number().divide(vargs[1].number())) return -1;
		if(vargs[1].number().isNegative() && vargs[0].number().isNonZero()) {
			if(vargs[0].number().isNegative()) {
				mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct.add(-180); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct.add(-200); break;}
					case ANGLE_UNIT_RADIANS: {mstruct.subtract(CALCULATOR->v_pi); break;}
					default: {MathStructure msub(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) msub *= CALCULATOR->getRadUnit(); mstruct.subtract(msub);}
				}
			} else {
				mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct.add(180); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct.add(200); break;}
					case ANGLE_UNIT_RADIANS: {mstruct.add(CALCULATOR->v_pi); break;}
					default: {MathStructure madd(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) madd *= CALCULATOR->getRadUnit(); mstruct.add(madd);}
				}
			}
		} else {
			mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
		}
	}
	return 1;
}
ArgFunction::ArgFunction() : MathFunction("arg", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool ArgFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool ArgFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
int ArgFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		if(!mstruct.number().hasImaginaryPart()) {
			if(!mstruct.number().isNonZero()) return -1;
			if(mstruct.number().isNegative()) {
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct.set(180, 1, 0); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct.set(200, 1, 0); break;}
					case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->v_pi); break;}
					default: {mstruct.set(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
				}
			} else {
				mstruct.clear();
			}
		} else if(!mstruct.number().hasRealPart() && mstruct.number().imaginaryPartIsNonZero()) {
			bool b_neg = mstruct.number().imaginaryPartIsNegative();
			switch(eo.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {mstruct.set(90, 1, 0); break;}
				case ANGLE_UNIT_GRADIANS: {mstruct.set(100, 1, 0); break;}
				case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->v_pi); mstruct.multiply(nr_half); break;}
				default: {mstruct.set(CALCULATOR->v_pi); mstruct.multiply(nr_half); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
			}
			if(b_neg) mstruct.negate();
		} else if(!mstruct.number().realPartIsNonZero()) {
			FR_FUNCTION(arg)
		} else {
			MathStructure new_nr(mstruct.number().imaginaryPart());
			if(!new_nr.number().divide(mstruct.number().realPart())) return -1;
			if(mstruct.number().realPartIsNegative()) {
				if(mstruct.number().imaginaryPartIsNegative()) {
					mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.add(-180); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.add(-200); break;}
						case ANGLE_UNIT_RADIANS: {mstruct.subtract(CALCULATOR->v_pi); break;}
						default: {MathStructure msub(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) msub *= CALCULATOR->getRadUnit(); mstruct.subtract(msub);}
					}
				} else {
					mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.add(180); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.add(200); break;}
						case ANGLE_UNIT_RADIANS: {mstruct.add(CALCULATOR->v_pi); break;}
						default: {MathStructure madd(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) madd *= CALCULATOR->getRadUnit(); mstruct.add(madd);}
					}
				}
			} else {
				mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
			}
		}
		return 1;
	} else {
		if(mstruct.representsPositive(true)) {
			mstruct.clear();
			return 1;
		}
		if(mstruct.representsNegative(true)) {
			switch(eo.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {mstruct.set(180, 1, 0); break;}
				case ANGLE_UNIT_GRADIANS: {mstruct.set(200, 1, 0); break;}
				case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->v_pi); break;}
				default: {mstruct.set(CALCULATOR->v_pi); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
			}
			return 1;
		}
		if(mstruct.isMultiplication()) {
			bool b = false;
			for(size_t i = 0; i < mstruct.size();) {
				if(mstruct[i].representsPositive()) {
					mstruct.delChild(i + 1);
					b = true;
				} else {
					if(!mstruct.isMinusOne() && mstruct[i].representsNegative()) {
						mstruct[i].set(-1, 1, 0, true);
						b = true;
					}
					i++;
				}
			}
			if(b) {
				if(mstruct.size() == 1) {
					mstruct.setToChild(1);
				} else if(mstruct.size() == 0) {
					mstruct.clear(true);
				}
				mstruct.transform(STRUCT_FUNCTION);
				mstruct.setFunction(this);
				return 1;
			}
		}
		if(mstruct.isPower() && mstruct[0].representsComplex() && mstruct[1].representsInteger()) {
			mstruct.setType(STRUCT_MULTIPLICATION);
			mstruct[0].transform(STRUCT_FUNCTION);
			mstruct[0].setFunction(this);
			return 1;
		}
	}
	return -1;
}

SincFunction::SincFunction() : MathFunction("sinc", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool SincFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsNumber() || is_number_angle_value(vargs[0]));}
bool SincFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsReal() || is_real_angle_value(vargs[0]));}
int SincFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.containsType(STRUCT_UNIT) && CALCULATOR->getRadUnit()) {
		mstruct.convert(CALCULATOR->getRadUnit());
		mstruct /= CALCULATOR->getRadUnit();
	}
	if(mstruct.isZero()) {
		mstruct.set(1, 1, 0, true);
		return 1;
	} else if(mstruct.representsNonZero(true)) {
		MathStructure *m_sin = new MathStructure(CALCULATOR->f_sin, &mstruct, NULL);
		if(CALCULATOR->getRadUnit()) (*m_sin)[0].multiply(CALCULATOR->getRadUnit());
		mstruct.inverse();
		mstruct.multiply_nocopy(m_sin);
		return 1;
	}
	return -1;
}


IntervalFunction::IntervalFunction() : MathFunction("interval", 2) {
	NumberArgument *arg = new NumberArgument();
	arg->setTests(false);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument();
	arg->setTests(false);
	setArgumentDefinition(2, arg);
}
int IntervalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isNumber() && vargs[1].isNumber()) {
		Number nr;
		if(!nr.setInterval(vargs[0].number(), vargs[1].number())) return 0;
		mstruct = nr;
		return 1;
	}
	MathStructure margs(vargs);
	margs.eval(eo);
	if(margs[0] == margs[1]) {
		mstruct = margs[0];
		return 1;
	} else if(margs[0].isMultiplication() && margs[1].isMultiplication() && margs[0].size() > 1 && margs[1].size() > 1) {
		size_t i0 = 0, i1 = 0;
		if(margs[0][0].isNumber()) i0++;
		if(margs[1][0].isNumber()) i1++;
		if(i0 == 0 && i1 == 0) {
			if(margs[0] == margs[1]) {
				mstruct = margs[0];
				return 1;
			}
			return 0;
		}
		if(margs[0].size() - i0 != margs[1].size() - i1) return 0;
		for(size_t i = 0; i < margs[0].size() - i0; i++) {
			if(margs[0][i + i0] != margs[1][i + i1]) {
				return 0;
			}
		}
		Number nr;
		if(!nr.setInterval(i0 == 1 ? margs[0][0].number() : nr_one, i1 == 1 ? margs[1][0].number() : nr_one)) return 0;
		mstruct = margs[0];
		if(i0 == 1) mstruct.delChild(1, true);
		mstruct *= nr;
		mstruct.evalSort(false);
		return 1;
	} else if(margs[0].isMultiplication() && margs[0].size() == 2 && margs[0][0].isNumber() && margs[1] == margs[0][1]) {
		Number nr;
		if(!nr.setInterval(margs[0][0].number(), nr_one)) return 0;
		mstruct = nr;
		mstruct *= margs[1];
		return 1;
	} else if(margs[1].isMultiplication() && margs[1].size() == 2 && margs[1][0].isNumber() && margs[0] == margs[1][1]) {
		Number nr;
		if(!nr.setInterval(nr_one, margs[1][0].number())) return 0;
		mstruct = nr;
		mstruct *= margs[0];
		return 1;
	} else if(margs[0].isNumber() && margs[1].isNumber()) {
		Number nr;
		if(!nr.setInterval(margs[0].number(), margs[1].number())) return 0;
		mstruct = nr;
		return 1;
	}
	return 0;
}


RadiansToDefaultAngleUnitFunction::RadiansToDefaultAngleUnitFunction() : MathFunction("radtodef", 1) {
}
int RadiansToDefaultAngleUnitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	switch(eo.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			mstruct *= 180;
	    		mstruct /= CALCULATOR->v_pi;
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			mstruct *= 200;
	    		mstruct /= CALCULATOR->v_pi;
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			break;
		}
		default: {
			if(CALCULATOR->getRadUnit()) {
				mstruct *= CALCULATOR->getRadUnit();
			}
		}
	}
	return 1;
}

TotalFunction::TotalFunction() : MathFunction("total", 1) {
	setArgumentDefinition(1, new VectorArgument(""));
}
int TotalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct.clear();
	for(size_t index = 0; index < vargs[0].size(); index++) {
		if(CALCULATOR->aborted()) return 0;
		mstruct.calculateAdd(vargs[0][index], eo);
	}
	return 1;
}
PercentileFunction::PercentileFunction() : MathFunction("percentile", 2, 3) {
	setArgumentDefinition(1, new VectorArgument(""));
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
				pfr.ceil();
				size_t index = pfr.uintValue();
				if(index > v.size()) index = v.size();
				if(index == 0) index = 1;
				mstruct = v[index - 1];
				return 1;
			}
			case 3: {
				pfr *= (long int) v.countChildren();
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
	setArgumentDefinition(1, new VectorArgument(""));
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
	setArgumentDefinition(1, new VectorArgument(""));
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
	setArgumentDefinition(1, new VectorArgument(""));
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
		if(is[index] > n) {
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
RandFunction::RandFunction() : MathFunction("rand", 0, 1) {
	setArgumentDefinition(1, new IntegerArgument());
	setDefaultValue(1, "0"); 
}
int RandFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	Number nr;
	if(vargs[0].number().isZero() || vargs[0].number().isNegative()) {
		nr.rand();
	} else {
		nr.intRand(vargs[0].number());
		nr++;
	}
	mstruct = nr;
	return 1;
}
bool RandFunction::representsReal(const MathStructure&, bool) const {return true;}
bool RandFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() > 0 && vargs[0].isNumber() && vargs[0].number().isPositive();}
bool RandFunction::representsNonNegative(const MathStructure&, bool) const {return true;}

ISODateFunction::ISODateFunction() : MathFunction("isodate", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int ISODateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	mstruct.set(date.toISOString());
	return 1;
}
LocalDateFunction::LocalDateFunction() : MathFunction("localdate", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int LocalDateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	mstruct.set(date.toLocalString());
	return 1;
}
TimestampFunction::TimestampFunction() : MathFunction("timestamp", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "now");
}
int TimestampFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	string str = vargs[0].symbol();
	remove_blank_ends(str);
	if(str == _("now") || str == "now") {
		mstruct.set((long int) time(NULL), 1L, 0L);
		return 1;
	}
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	Number nr(date.timestamp());
	if(nr.isInfinite()) return 0;
	mstruct.set(nr);
	return 1;
}
TimestampToDateFunction::TimestampToDateFunction() : MathFunction("stamptodate", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}
int TimestampToDateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {	
	long int i = vargs[0].number().lintValue();
	QalculateDate date;
	if(!date.set(i)) return false;
	mstruct.set(date.toISOString());
	return 1;
}

AddDaysFunction::AddDaysFunction() : MathFunction("addDays", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}	
int AddDaysFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	long int i = vargs[1].number().lintValue();
	if(!date.addDays(i)) return 0;
	mstruct.set(date.toISOString());
	return 1;
}
AddMonthsFunction::AddMonthsFunction() : MathFunction("addMonths", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}	
int AddMonthsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	long int i = vargs[1].number().lintValue();
	if(!date.addMonths(i)) return 0;
	mstruct.set(date.toISOString());
	return 1;
}
AddYearsFunction::AddYearsFunction() : MathFunction("addYears", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}	
int AddYearsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {	
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	long int i = vargs[1].number().lintValue();
	if(!date.addYears(i)) return 0;
	mstruct.set(date.toISOString());
	return 1;
}

DaysFunction::DaysFunction() : MathFunction("days", 2, 4) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new DateArgument());
	IntegerArgument *arg = new IntegerArgument();
	Number integ;
	arg->setMin(&integ);
	integ.set(4, 1, 0);
	arg->setMax(&integ);
	setArgumentDefinition(3, arg);	
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(3, "1"); 
}
int DaysFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date1, date2;
	if(!date1.set(vargs[0].symbol()) || !date2.set(vargs[1].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	Number days(date1.daysTo(date2, vargs[2].number().intValue(), vargs[3].number().isZero()));
	if(days.isInfinite()) return 0;
	days.abs();
	mstruct.set(days);
	return 1;
}
YearFracFunction::YearFracFunction() : MathFunction("yearfrac", 2, 4) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new DateArgument());
	IntegerArgument *arg = new IntegerArgument();
	Number integ;
	arg->setMin(&integ);
	integ.set(4, 1, 0);
	arg->setMax(&integ);
	setArgumentDefinition(3, arg);	
	setArgumentDefinition(4, new BooleanArgument());		
	setDefaultValue(3, "1");
}
int YearFracFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date1, date2;
	if(!date1.set(vargs[0].symbol()) || !date2.set(vargs[1].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	Number years(date1.yearsTo(date2, vargs[2].number().intValue(), vargs[3].number().isZero()));
	if(years.isInfinite()) return 0;
	years.abs();
	mstruct.set(years);
	return 1;
}
WeekFunction::WeekFunction() : MathFunction("week", 0, 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new BooleanArgument());	
	setDefaultValue(1, "today");
}
int WeekFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	int w = date.week(vargs[1].number().getBoolean());
	if(w < 0) return 0;
	mstruct.set(w, 1, 0);
	return 1;
}
WeekdayFunction::WeekdayFunction() : MathFunction("weekday", 0, 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(1, "today");
}
int WeekdayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	int w = date.weekday();
	if(w < 0) return 0;
	if(vargs[1].number().getBoolean()) {
		if(w == 7) w = 1;
		else w++;
	}
	mstruct.set(w, 1, 0);
	return 1;
}
YeardayFunction::YeardayFunction() : MathFunction("yearday", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int YeardayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	int yd = date.yearday();
	if(yd < 0) return 0;
	mstruct.set(yd, 1, 0);
	return 1;
}
MonthFunction::MonthFunction() : MathFunction("month", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int MonthFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	mstruct.set(date.month(), 1L, 0L);
	return 1;
}
DayFunction::DayFunction() : MathFunction("day", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int DayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	mstruct.set(date.day(), 1L, 0L);
	return 1;
}
YearFunction::YearFunction() : MathFunction("year", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int YearFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDate date;
	if(!date.set(vargs[0].symbol())) {
		CALCULATOR->error(true, _("Error in date format for function %s()."), name().c_str(), NULL);
		return 0;
	}
	mstruct.set(date.year(), 1L, 0L);
	return 1;
}
TimeFunction::TimeFunction() : MathFunction("time", 0) {
}
int TimeFunction::calculate(MathStructure &mstruct, const MathStructure&, const EvaluationOptions&) {
	int hour, min, sec;
	now(hour, min, sec);
	Number tnr(sec, 1, 0);
	tnr /= 60;
	tnr += min;
	tnr /= 60;
	tnr += hour;
	mstruct = tnr;
	return 1;
}

BinFunction::BinFunction() : MathFunction("bin", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int BinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//mstruct = Number(vargs[0].symbol(), 2);
	ParseOptions po = eo.parse_options;
	po.base = BASE_BINARY;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
OctFunction::OctFunction() : MathFunction("oct", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int OctFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//mstruct = Number(vargs[0].symbol(), 8);
	ParseOptions po = eo.parse_options;
	po.base = BASE_OCTAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
HexFunction::HexFunction() : MathFunction("hex", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int HexFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//mstruct = Number(vargs[0].symbol(), 16);
	ParseOptions po = eo.parse_options;
	po.base = BASE_HEXADECIMAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
BaseFunction::BaseFunction() : MathFunction("base", 2) {
	setArgumentDefinition(1, new TextArgument());
	IntegerArgument *arg = new IntegerArgument();
	Number integ(2, 1, 0);
	arg->setMin(&integ);
	integ.set(36, 1, 0);
	arg->setMax(&integ);
	setArgumentDefinition(2, arg);
}
int BaseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//mstruct = Number(vargs[0].symbol(), vargs[1].number().intValue());
	ParseOptions po = eo.parse_options;
	po.base = vargs[1].number().intValue();
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
RomanFunction::RomanFunction() : MathFunction("roman", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int RomanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//mstruct = Number(vargs[0].symbol(), BASE_ROMAN_NUMERALS);
	ParseOptions po = eo.parse_options;
	po.base = BASE_ROMAN_NUMERALS;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}

AsciiFunction::AsciiFunction() : MathFunction("code", 1) {
	TextArgument *arg = new TextArgument();
	arg->setCustomCondition("len(\\x) = 1");
	setArgumentDefinition(1, arg);
}
int AsciiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	unsigned char c = (unsigned char) vargs[0].symbol()[0];
	mstruct.set(c, 1, 0);
	return 1;
}
CharFunction::CharFunction() : MathFunction("char", 1) {
	IntegerArgument *arg = new IntegerArgument();
	Number fr(32, 1, 0);
	arg->setMin(&fr);
	fr.set(0x7f, 1, 0);
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
int CharFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	string str;
	str += vargs[0].number().intValue();
	mstruct = str;
	return 1;
}

ConcatenateFunction::ConcatenateFunction() : MathFunction("concatenate", 1, -1) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new TextArgument());
}
int ConcatenateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	string str;
	for(size_t i = 0; i < vargs.size(); i++) {
		str += vargs[i].symbol();
	}
	mstruct = str;
	return 1;
}
LengthFunction::LengthFunction() : MathFunction("len", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int LengthFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) vargs[0].symbol().length();
	return 1;
}

ReplaceFunction::ReplaceFunction() : MathFunction("replace", 3, 4) {
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(4, "0");
}
int ReplaceFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	if(vargs[3].number().getBoolean()) mstruct.eval(eo);
	mstruct.replace(vargs[1], vargs[2]);
	return 1;
}
StripUnitsFunction::StripUnitsFunction() : MathFunction("nounit", 1) {
}
int StripUnitsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	mstruct.removeType(STRUCT_UNIT);
	return 1;
}

ErrorFunction::ErrorFunction() : MathFunction("error", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int ErrorFunction::calculate(MathStructure&, const MathStructure &vargs, const EvaluationOptions&) {
	CALCULATOR->error(true, vargs[0].symbol().c_str(), NULL);
	return 1;
}
WarningFunction::WarningFunction() : MathFunction("warning", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int WarningFunction::calculate(MathStructure&, const MathStructure &vargs, const EvaluationOptions&) {
	CALCULATOR->error(false, vargs[0].symbol().c_str(), NULL);
	return 1;
}
MessageFunction::MessageFunction() : MathFunction("message", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int MessageFunction::calculate(MathStructure&, const MathStructure &vargs, const EvaluationOptions&) {
	CALCULATOR->message(MESSAGE_INFORMATION, vargs[0].symbol().c_str(), NULL);
	return 1;
}

GenerateVectorFunction::GenerateVectorFunction() : MathFunction("genvector", 4, 6) {
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(5, "x");
	setArgumentDefinition(6, new BooleanArgument());
	setDefaultValue(6, "0");
}
int GenerateVectorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(CALCULATOR->aborted()) return 0;
	if(vargs[5].number().getBoolean()) {
		mstruct = vargs[0].generateVector(vargs[4], vargs[1], vargs[2], vargs[3], NULL, eo);
	} else {
		bool overflow = false;
		int steps = vargs[3].number().intValue(&overflow);
		if(!vargs[3].isNumber() || overflow || steps < 1) {
			CALCULATOR->error(true, _("The number of requested elements in generate vector function must be a positive integer."), NULL);
			return 0;
		}
		mstruct = vargs[0].generateVector(vargs[4], vargs[1], vargs[2], steps, NULL, eo);
	}
	if(CALCULATOR->aborted()) return 0;
	return 1;
}
ForFunction::ForFunction() : MathFunction("for", 7) {
	setArgumentDefinition(2, new SymbolicArgument());	
	setArgumentDefinition(7, new SymbolicArgument());
}
int ForFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[4];
	MathStructure mcounter = vargs[0];
	MathStructure mtest;
	MathStructure mcount;
	MathStructure mupdate;
	while(true) {
		mtest = vargs[2];
		mtest.replace(vargs[1], mcounter);
		mtest.eval(eo);
		if(!mtest.isNumber() || CALCULATOR->aborted()) return 0;
		if(!mtest.number().getBoolean()) {
			break;
		}
		
		mupdate = vargs[5];
		mupdate.replace(vargs[1], mcounter, vargs[6], mstruct);
		mstruct = mupdate;
		mstruct.calculatesub(eo, eo, false);
		
		mcount = vargs[3];
		mcount.replace(vargs[1], mcounter);
		mcounter = mcount;
		mcounter.calculatesub(eo, eo, false);
	}
	return 1;

}
SumFunction::SumFunction() : MathFunction("sum", 3, 4) {
	setArgumentDefinition(2, new IntegerArgument());
	setArgumentDefinition(3, new IntegerArgument());	
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "x");
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
	CALCULATOR->beginTemporaryStopMessages();
	m1.eval(eo2);
	int im = 0;
	CALCULATOR->endTemporaryStopMessages(&im);
	if(im > 0) m1 = vargs[0];
	eo2.calculate_functions = eo.calculate_functions;
	eo2.expand = eo.expand;
	mstruct.clear();
	MathStructure mstruct_calc;
	bool started = false;
	while(i_nr.isLessThanOrEqualTo(vargs[2].number())) {
		if(CALCULATOR->aborted()) {
			if(!started) {
				return 0;
			} else if(i_nr != vargs[2].number()) {
				MathStructure mmin(i_nr);
				mstruct.add(MathStructure(this, &vargs[0], &mmin, &vargs[2], &vargs[3], NULL), true);
				return 1;
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
	return 1;
	
}
ProductFunction::ProductFunction() : MathFunction("product", 3, 4) {
	setArgumentDefinition(2, new IntegerArgument());
	setArgumentDefinition(3, new IntegerArgument());	
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "x");
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
	CALCULATOR->beginTemporaryStopMessages();
	m1.eval(eo2);
	int im = 0;
	CALCULATOR->endTemporaryStopMessages(&im);
	if(im > 0) m1 = vargs[0];
	eo2.calculate_functions = eo.calculate_functions;
	eo2.expand = eo.expand;
	mstruct.clear();
	MathStructure mstruct_calc;
	bool started = false;
	while(i_nr.isLessThanOrEqualTo(vargs[2].number())) {
		if(CALCULATOR->aborted()) {
			if(!started) {
				return 0;
			} else if(i_nr != vargs[2].number()) {
				MathStructure mmin(i_nr);
				mstruct.multiply(MathStructure(this, &vargs[0], &mmin, &vargs[2], &vargs[3], NULL), true);
				return 1;
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
	return 1;
	
}

bool process_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t index);
bool process_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t index) {
	if(mprocess == vargs[1]) {
		mprocess = mstruct[index];
		return true;
	}
	if(!vargs[3].isEmptySymbol() && mprocess == vargs[3]) {
		mprocess = (int) index + 1;
		return true;
	}
	if(!vargs[4].isEmptySymbol() && mprocess == vargs[4]) {
		mprocess = vargs[2];
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < mprocess.size(); i++) {
		if(process_replace(mprocess[i], mstruct, vargs, index)) {
			mprocess.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}

ProcessFunction::ProcessFunction() : MathFunction("process", 3, 5) {
	setArgumentDefinition(2, new SymbolicArgument());
	setArgumentDefinition(3, new VectorArgument());
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "\"\"");
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(5, "\"\"");
}
int ProcessFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {

	mstruct = vargs[2];
	MathStructure mprocess;
	for(size_t index = 0; index < mstruct.size(); index++) {
		mprocess = vargs[0];
		process_replace(mprocess, mstruct, vargs, index);
		mstruct[index] = mprocess;
	}
	return 1;
	
}


bool process_matrix_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t rindex, size_t cindex);
bool process_matrix_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t rindex, size_t cindex) {
	if(mprocess == vargs[1]) {
		mprocess = mstruct[rindex][cindex];
		return true;
	}
	if(!vargs[3].isEmptySymbol() && mprocess == vargs[3]) {
		mprocess = (int) rindex + 1;
		return true;
	}
	if(!vargs[4].isEmptySymbol() && mprocess == vargs[4]) {
		mprocess = (int) cindex + 1;
		return true;
	}
	if(!vargs[5].isEmptySymbol() && mprocess == vargs[5]) {
		mprocess = vargs[2];
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < mprocess.size(); i++) {
		if(process_matrix_replace(mprocess[i], mstruct, vargs, rindex, cindex)) {
			mprocess.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}

ProcessMatrixFunction::ProcessMatrixFunction() : MathFunction("processm", 3, 6) {
	setArgumentDefinition(2, new SymbolicArgument());
	setArgumentDefinition(3, new MatrixArgument());
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "\"\"");
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(5, "\"\"");
	setArgumentDefinition(6, new SymbolicArgument());
	setDefaultValue(6, "\"\"");
}
int ProcessMatrixFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {

	mstruct = vargs[2];
	MathStructure mprocess;
	for(size_t rindex = 0; rindex < mstruct.size(); rindex++) {
		for(size_t cindex = 0; cindex < mstruct[rindex].size(); cindex++) {
			if(CALCULATOR->aborted()) return 0;
			mprocess = vargs[0];
			process_matrix_replace(mprocess, mstruct, vargs, rindex, cindex);
			mstruct[rindex][cindex] = mprocess;
		}
	}
	return 1;
	
}

bool csum_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t index, const EvaluationOptions &eo2);
bool csum_replace(MathStructure &mprocess, const MathStructure &mstruct, const MathStructure &vargs, size_t index, const EvaluationOptions &eo2) {
	if(mprocess == vargs[4]) {
		mprocess = vargs[6][index];
		return true;
	}
	if(mprocess == vargs[5]) {
		mprocess = mstruct;
		return true;
	}
	if(!vargs[7].isEmptySymbol() && mprocess == vargs[7]) {
		mprocess = (int) index + 1;
		return true;
	}
	if(!vargs[8].isEmptySymbol()) {
		if(mprocess.isFunction() && mprocess.function() == CALCULATOR->f_component && mprocess.size() == 2 && mprocess[1] == vargs[8]) {
			bool b = csum_replace(mprocess[0], mstruct, vargs, index, eo2);
			mprocess[0].eval(eo2);
			if(mprocess[0].isNumber() && mprocess[0].number().isInteger() && mprocess[0].number().isPositive() && mprocess[0].number().isLessThanOrEqualTo(vargs[6].size())) {
				mprocess = vargs[6][mprocess[0].number().intValue() - 1];
				return true;
			}
			return csum_replace(mprocess[1], mstruct, vargs, index, eo2) || b;
		} else if(mprocess == vargs[8]) {
			mprocess = vargs[6];
			return true;
		}
	}
	bool b = false;
	for(size_t i = 0; i < mprocess.size(); i++) {
		if(csum_replace(mprocess[i], mstruct, vargs, index, eo2)) {
			mprocess.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
CustomSumFunction::CustomSumFunction() : MathFunction("csum", 7, 9) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SINT)); //begin
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT)); //end
	//3. initial
	//4. function
	setArgumentDefinition(5, new SymbolicArgument()); //x var
	setArgumentDefinition(6, new SymbolicArgument()); //y var
	setArgumentDefinition(7, new VectorArgument());
	setArgumentDefinition(8, new SymbolicArgument()); //i var
	setDefaultValue(8, "\"\"");
	setArgumentDefinition(9, new SymbolicArgument()); //v var
	setDefaultValue(9, "\"\"");
}
int CustomSumFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	int start = vargs[0].number().intValue();
	if(start < 1) start = 1;
	int end = vargs[1].number().intValue();
	int n = vargs[6].countChildren();
	if(start > n) {
		CALCULATOR->error(true, _("Too few elements (%s) in vector (%s required)"), i2s(n).c_str(), i2s(start).c_str(), NULL);
		start = n;
	}
	if(end < 1 || end > n) {
		if(end > n) CALCULATOR->error(true, _("Too few elements (%s) in vector (%s required)"), i2s(n).c_str(), i2s(end).c_str(), NULL);
		end = n;
	} else if(end < start) {
		end = start;
	}
	
	mstruct = vargs[2];
	MathStructure mexpr(vargs[3]);
	MathStructure mprocess;
	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	mstruct.eval(eo2);
	for(size_t index = (size_t) start - 1; index < (size_t) end; index++) {
		if(CALCULATOR->aborted()) return 0;
		mprocess = mexpr;
		csum_replace(mprocess, mstruct, vargs, index, eo2);
		mprocess.eval(eo2);
		mstruct = mprocess;
	}
	return 1;

}

FunctionFunction::FunctionFunction() : MathFunction("function", 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new VectorArgument());
}
int FunctionFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	UserFunction f = new UserFunction("", "Generated MathFunction", vargs[0].symbol());
	MathStructure args = vargs[1];
	mstruct = f.MathFunction::calculate(args, eo);
	if(mstruct.isFunction() && mstruct.function() == &f) mstruct.setUndefined();
	return 1;
}
SelectFunction::SelectFunction() : MathFunction("select", 2, 4) {
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "x");
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(4, "0");
}
int SelectFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure mtest;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(!mstruct.isVector()) {
		mtest = vargs[1];
		mtest.replace(vargs[2], mstruct);
		mtest.eval(eo);
		if(!mtest.isNumber() || mtest.number().getBoolean() < 0) {
			CALCULATOR->error(true, _("Comparison failed."), NULL);
			return -1;
		}
		if(mtest.number().getBoolean() == 0) {
			if(vargs[3].number().getBoolean() > 0) {
				CALCULATOR->error(true, _("No matching item found."), NULL);
				return -1;
			}
			mstruct.clearVector();
		}
		return 1;
	}
	for(size_t i = 0; i < mstruct.size();) {
		mtest = vargs[1];
		mtest.replace(vargs[2], mstruct[i]);
		mtest.eval(eo);
		if(!mtest.isNumber() || mtest.number().getBoolean() < 0) {
			CALCULATOR->error(true, _("Comparison failed."), NULL);
			return -1;
		}
		if(mtest.number().getBoolean() == 0) {
			if(vargs[3].number().getBoolean() == 0) {
				mstruct.delChild(i + 1);
			} else {
				i++;
			}
		} else if(vargs[3].number().getBoolean() > 0) {
			MathStructure msave(mstruct[i]);
			mstruct = msave;
			return 1;
		} else {
			i++;
		}
	}
	if(vargs[3].number().getBoolean() > 0) {
		CALCULATOR->error(true, _("No matching item found."), NULL);
		return -1;
	}
	return 1;
}
IFFunction::IFFunction() : MathFunction("if", 3) {
	NON_COMPLEX_NUMBER_ARGUMENT(1)
}
int IFFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	int result = vargs[0].number().getBoolean();
	if(result) {			
		mstruct = vargs[1];
	} else if(result == 0) {			
		mstruct = vargs[2];
	} else {
		return 0;
	}	
	return 1;
}

IsNumberFunction::IsNumberFunction() : MathFunction("isNumber", 1) {
}
int IsNumberFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	if(!mstruct.isNumber()) mstruct.eval(eo);
	if(mstruct.isNumber()) {
		mstruct.number().setTrue();
	} else {
		mstruct.clear();
		mstruct.number().setFalse();
	}
	return 1;
}
IS_NUMBER_FUNCTION(IsIntegerFunction, isInteger)
IS_NUMBER_FUNCTION(IsRealFunction, isReal)
IS_NUMBER_FUNCTION(IsRationalFunction, isRational)
REPRESENTS_FUNCTION(RepresentsIntegerFunction, representsInteger)
REPRESENTS_FUNCTION(RepresentsRealFunction, representsReal)
REPRESENTS_FUNCTION(RepresentsRationalFunction, representsRational)
REPRESENTS_FUNCTION(RepresentsNumberFunction, representsNumber)

LoadFunction::LoadFunction() : MathFunction("load", 1, 3) {
	setArgumentDefinition(1, new FileArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SINT));
	setDefaultValue(2, "1");
	setArgumentDefinition(3, new TextArgument());
	setDefaultValue(3, ",");	
}
int LoadFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	string delim = vargs[2].symbol();
	if(delim == "tab") {
		delim = "\t";
	}
	if(!CALCULATOR->importCSV(mstruct, vargs[0].symbol().c_str(), vargs[1].number().intValue(), delim)) {
		CALCULATOR->error(true, "Failed to load %s.", vargs[0].symbol().c_str(), NULL);
		return 0;
	}
	return 1;
}
ExportFunction::ExportFunction() : MathFunction("export", 2, 3) {
	setArgumentDefinition(1, new VectorArgument());
	setArgumentDefinition(2, new FileArgument());
	setArgumentDefinition(3, new TextArgument());
	setDefaultValue(3, ",");	
}
int ExportFunction::calculate(MathStructure&, const MathStructure &vargs, const EvaluationOptions&) {
	string delim = vargs[2].symbol();
	if(delim == "tab") {
		delim = "\t";
	}
	if(!CALCULATOR->exportCSV(vargs[0], vargs[1].symbol().c_str(), delim)) {
		CALCULATOR->error(true, "Failed to export to %s.", vargs[1].symbol().c_str(), NULL);
		return 0;
	}
	return 1;
}
TitleFunction::TitleFunction() : MathFunction("title", 1) {
	setArgumentDefinition(1, new ExpressionItemArgument());
}
int TitleFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	ExpressionItem *item = CALCULATOR->getExpressionItem(vargs[0].symbol());
	if(!item) {
		CALCULATOR->error(true, _("Object %s does not exist."), vargs[0].symbol().c_str(), NULL);
		return 0;
	} else {
		mstruct = item->title();
	}
	return 1;
}
SaveFunction::SaveFunction() : MathFunction("save", 2, 4) {
	setArgumentDefinition(2, new TextArgument());
	setArgumentDefinition(3, new TextArgument());
	setArgumentDefinition(4, new TextArgument());
	setDefaultValue(3, CALCULATOR->temporaryCategory());
	setDefaultValue(4, "");	
}
int SaveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(!CALCULATOR->variableNameIsValid(vargs[1].symbol())) {
		CALCULATOR->error(true, _("Invalid variable name (%s)."), vargs[1].symbol().c_str(), NULL);
		return -1;
	}
	if(CALCULATOR->variableNameTaken(vargs[1].symbol())) {
		Variable *v = CALCULATOR->getActiveVariable(vargs[1].symbol());
		if(v && v->isLocal() && v->isKnown()) {
			if(!vargs[2].symbol().empty()) v->setCategory(vargs[2].symbol());
			if(!vargs[3].symbol().empty()) v->setTitle(vargs[3].symbol());
			((KnownVariable*) v)->set(mstruct);
			if(v->countNames() == 0) {
				ExpressionName ename(vargs[1].symbol());
				ename.reference = true;
				v->setName(ename, 1);
			} else {
				v->setName(vargs[1].symbol(), 1);
			}
		} else {
			CALCULATOR->error(false, _("A global unit or variable was deactivated. It will be restored after the new variable has been removed."), NULL);
			CALCULATOR->addVariable(new KnownVariable(vargs[2].symbol(), vargs[1].symbol(), mstruct, vargs[3].symbol()));
		}
	} else {
		CALCULATOR->addVariable(new KnownVariable(vargs[2].symbol(), vargs[1].symbol(), mstruct, vargs[3].symbol()));
	}
	CALCULATOR->saveFunctionCalled();
	return 1;
}

RegisterFunction::RegisterFunction() : MathFunction("register", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int RegisterFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].number().isGreaterThan(CALCULATOR->RPNStackSize())) {
		CALCULATOR->error(false, _("Register %s does not exist. Returning zero."), vargs[0].print().c_str(), NULL);
		mstruct.clear();
		return 1;
	}
	mstruct.set(*CALCULATOR->getRPNRegister((size_t) vargs[0].number().uintValue()));
	return 1;
}
StackFunction::StackFunction() : MathFunction("stack", 0) {
}
int StackFunction::calculate(MathStructure &mstruct, const MathStructure&, const EvaluationOptions&) {
	mstruct.clearVector();
	for(size_t i = 1; i <= CALCULATOR->RPNStackSize(); i++) {
		mstruct.addChild(*CALCULATOR->getRPNRegister(i));
	}
	return 1;
}

DeriveFunction::DeriveFunction() : MathFunction("diff", 1, 3) {
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SINT));
	setDefaultValue(3, "1");		
}
int DeriveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	int i = vargs[2].number().intValue();
	mstruct = vargs[0];
	bool b = false;
	while(i) {
		if(i > 0) mstruct.calculatesub(eo, eo, true);
		if(CALCULATOR->aborted()) return 0;
		if(!mstruct.differentiate(vargs[1], eo) && !b) {
			return 0;
		}
		b = true;
		i--;
	}
	return 1;
}
IntegrateFunction::IntegrateFunction() : MathFunction("integrate", 1, 4) {
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
	setDefaultValue(3, "undefined");
	setDefaultValue(4, "undefined");
}
int IntegrateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	//CALCULATOR->error(false, _("The %s function is incomplete, unreliable and unstable. Use at your own risk and avoid complex expressions."), preferredDisplayName().name.c_str(), NULL);
	mstruct = vargs[0];
	if(!mstruct.integrate(vargs[1], eo)) {
		mstruct = vargs[0];
		mstruct.eval(eo);
		if(mstruct == vargs[0]) {
			return 0;
		}
		MathStructure mstruct2(mstruct);
		if(!mstruct.integrate(vargs[1], eo)) {
			mstruct = mstruct2;
			CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
			return -1;
		}
		mstruct += "C";
	} else {
		mstruct += "C";
	}
	if(!vargs[2].isUndefined()) {
		if(vargs[3].isUndefined()) {
			CALCULATOR->error(true, _("Both the lower and upper limit must be set to get the definite integral."), NULL);
			mstruct = vargs[0];
			return -1;
		}
		if(mstruct.containsFunction(this, false, true, true) > 0) {
			CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
			mstruct = vargs[0];
			return -1;
		}
		MathStructure mstruct_lower(mstruct);
		mstruct_lower.replace(vargs[1], vargs[2]);
		mstruct.replace(vargs[1], vargs[3]);
		mstruct -= mstruct_lower;
	}
	return 1;
}
SolveFunction::SolveFunction() : MathFunction("solve", 1, 2) {
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "x");
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

MathStructure *solve_handle_logical_and(MathStructure &mstruct, MathStructure **mtruefor, ComparisonType ct, bool &b_partial, const MathStructure &vargs) {
	MathStructure *mcondition = NULL;
	for(size_t i2 = 0; i2 < mstruct.size(); ) {
		if(ct == COMPARISON_EQUALS) {
			if(mstruct[i2].isComparison() && ct == mstruct[i2].comparisonType() && mstruct[i2][0].contains(vargs[1])) {
				if(mstruct[i2][0] == vargs[1]) {
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
			if(mstruct[i2].isComparison() && mstruct[i2][0].contains(vargs[1])) {
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

int SolveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

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
				CALCULATOR->error(true, _("No equality or inequality to solve. The entered expression to solve is not correct (ex. \"x + 5 = 3\" is correct)"), NULL);
				return -1;
			} else {
				first_error = ierror;
				msave = mstruct;
			}
		}
		
		itry++;
		
		if(itry == 2) {
			if(vargs[1].isVariable() && vargs[1].variable()->subtype() == SUBTYPE_UNKNOWN_VARIABLE) {
				assumptions = ((UnknownVariable*) vargs[1].variable())->assumptions();
				if(!assumptions) {
					assumptions = new Assumptions();
					assumptions->setSign(CALCULATOR->defaultAssumptions()->sign());
					assumptions->setType(CALCULATOR->defaultAssumptions()->type());
					((UnknownVariable*) vargs[1].variable())->setAssumptions(assumptions);
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
			if(assumptions_added) ((UnknownVariable*) vargs[1].variable())->setAssumptions(NULL);
			switch(first_error) {
				case 2: {
					CALCULATOR->error(true, _("The comparison is true for all %s (with current assumptions)."), vargs[1].print().c_str(), NULL);
					break;
				}
				case 3: {
					CALCULATOR->error(true, _("No possible solution was found (with current assumptions)."), NULL);
					break;
				}
				case 4: {
					CALCULATOR->error(true, _("Was unable to completely isolate %s."), vargs[1].print().c_str(), NULL);
					break;
				}
				case 7: {					
					CALCULATOR->error(false, _("The comparison is true for all %s if %s."), vargs[1].print().c_str(), strueforall.c_str(), NULL);
					break;
				}
				default: {
					CALCULATOR->error(true, _("Was unable to isolate %s."), vargs[1].print().c_str(), NULL);
					break;
				}
			}
			mstruct = msave;
			return -1;
		}

		ComparisonType ct = COMPARISON_EQUALS;
	
		bool b = false;
		bool b_partial = false;

		if(vargs[0].isComparison()) {
			ct = vargs[0].comparisonType();
			mstruct = vargs[0];
			b = true;
		} else if(vargs[0].isLogicalAnd() && vargs[0].size() > 0 && vargs[0][0].isComparison()) {
			ct = vargs[0][0].comparisonType();
			mstruct = vargs[0];
			b = true;
		} else if(vargs[0].isLogicalOr() && vargs[0].size() > 0 && vargs[0][0].isComparison()) {
			ct = vargs[0][0].comparisonType();
			mstruct = vargs[0];
			b = true;
		} else if(vargs[0].isLogicalOr() && vargs[0].size() > 0 && vargs[0][0].isLogicalAnd() && vargs[0][0].size() > 0 && vargs[0][0][0].isComparison()) {
			ct = vargs[0][0][0].comparisonType();
			mstruct = vargs[0];
			b = true;
		} else if(vargs[0].isVariable() && vargs[0].variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || !vargs[0].variable()->isApproximate()) && ((KnownVariable*) vargs[0].variable())->get().isComparison()) {
			mstruct = ((KnownVariable*) vargs[0].variable())->get();
			ct = vargs[0].comparisonType();
			b = true;
		} else {
			EvaluationOptions eo2 = eo;
			eo2.test_comparisons = false;
			eo2.assume_denominators_nonzero = false;
			eo2.isolate_x = false;
			mstruct = vargs[0];
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
		eo2.isolate_var = &vargs[1];
		eo2.isolate_x = true;
		eo2.test_comparisons = true;
		mstruct.eval(eo2);

		if(mstruct.isOne()) {		
			ierror = 2;
			continue;
		} else if(mstruct.isZero()) {		
			ierror = 3;
			continue;
		}
	
		PrintOptions po;
		po.spell_out_logical_operators = true;
	
		if(mstruct.isComparison()) {
			if((ct == COMPARISON_EQUALS && mstruct.comparisonType() != COMPARISON_EQUALS) || !mstruct.contains(vargs[1])) {
				if(itry == 1) {
					mstruct.format(po);
					strueforall = mstruct.print(po);
				}
				ierror = 7;
				continue;
			} else if(ct == COMPARISON_EQUALS && mstruct[0] != vargs[1]) {
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
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) vargs[1].variable())->setAssumptions(NULL);
			}
			return 1;
		} else if(mstruct.isLogicalAnd()) {
			MathStructure *mtruefor = NULL;
			bool b_partial;
			MathStructure mcopy(mstruct);
			MathStructure *mcondition = solve_handle_logical_and(mstruct, &mtruefor, ct, b_partial, vargs);
			if((!mstruct.isComparison() && !mstruct.isLogicalAnd()) || (ct == COMPARISON_EQUALS && (!mstruct.isComparison() || mstruct.comparisonType() != COMPARISON_EQUALS || mstruct[0] != vargs[1])) || !mstruct.contains(vargs[1])) {
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
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) vargs[1].variable())->setAssumptions(NULL);
			}			
			if(mcondition) {
				mcondition->format(po);
				CALCULATOR->error(false, _("The solution requires that %s."), mcondition->print(po).c_str(), NULL);
				delete mcondition;
			}
			if(mtruefor) {
				mtruefor->format(po);
				CALCULATOR->error(false, _("The comparison is true for all %s if %s."), vargs[1].print().c_str(), mtruefor->print(po).c_str(), NULL);
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
					mcondition = solve_handle_logical_and(mstruct[i], &mtruefor, ct, b_partial, vargs);	
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
				if((!mstruct[i].isComparison() && !mstruct[i].isLogicalAnd()) || (ct == COMPARISON_EQUALS && (!mstruct[i].isComparison() || mstruct[i].comparisonType() != COMPARISON_EQUALS)) || !mstruct[i].contains(vargs[1])) {
					b_del = true;
				} else if(ct == COMPARISON_EQUALS && mstruct[i][0] != vargs[1]) {
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
					mstruct[i].setToChild(2, true);
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
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefor temporarily set as unknown."), vargs[1].print().c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) vargs[1].variable())->setAssumptions(NULL);
			}
			
			if(mconditions.size() == 1) {
				if(mconditions[0]) {
					mconditions[0]->format(po);
					CALCULATOR->error(false, _("The solution requires that %s."), mconditions[0]->print(po).c_str(), NULL);
					delete mconditions[0];
				}
			} else {
				string sconditions;
				for(size_t i = 0; i < mconditions.size(); i++) {
					if(mconditions[i]) {
						mconditions[i]->format(po);
						CALCULATOR->error(false, _("Solution %s requires that %s."), i2s(i + 1).c_str(), mconditions[i]->print(po).c_str(), NULL);
						delete mconditions[i];
					}				
				}
			}
			if(mtruefor) {
				mtruefor->format(po);
				CALCULATOR->error(false, _("The comparison is true for all %s if %s."), vargs[1].print().c_str(), mtruefor->print(po).c_str(), NULL);
				delete mtruefor;
			}
			return 1;
		} else {
			ierror = 6;
		}
	}
	return -1;
	
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
	
	for(size_t i = 0; i < eorder.size(); i++) {
		MathStructure msolve(vargs[0][eorder[i]]);
		EvaluationOptions eo2 = eo;
		eo2.isolate_var = &vargs[1][i];
		for(size_t i2 = 0; i2 < i; i2++) {
			msolve.replace(vargs[1][i2], mstruct[i2]);
		}
		msolve.eval(eo2);

		if(msolve.isComparison()) {
			if(msolve[0] != vargs[1][i]) {
				if(!b) {
					CALCULATOR->error(true, _("Unable to isolate %s.\n\nYou might need to place the equations and variables in an appropriate order so that each equation at least contains the corresponding variable (if automatic reordering failed)."), vargs[1][i].print().c_str(), NULL);
				} else {
					CALCULATOR->error(true, _("Unable to isolate %s."), vargs[1][i].print().c_str(), NULL);
				}
				return 0;
			} else {
				if(msolve.comparisonType() == COMPARISON_EQUALS) {
					mstruct.addChild(msolve[1]);
				} else {
					CALCULATOR->error(true, _("Inequalities is not allowed in %s()."), preferredName().name.c_str(), NULL);
					return 0;
				}
			}
		} else if(msolve.isLogicalOr()) {
			for(size_t i2 = 0; i2 < msolve.size(); i2++) {
				if(!msolve[i2].isComparison() || msolve[i2].comparisonType() != COMPARISON_EQUALS || msolve[i2][0] != vargs[1][i]) {
					CALCULATOR->error(true, _("Unable to isolate %s."), vargs[1][i].print().c_str(), NULL);
					return 0;
				} else {
					msolve[i2].setToChild(2, true);
				}
			}
			msolve.setType(STRUCT_VECTOR);
			mstruct.addChild(msolve);
		} else {
			CALCULATOR->error(true, _("Unable to isolate %s."), vargs[1][i].print().c_str(), NULL);
			return 0;
		}
		for(size_t i2 = 0; i2 < i; i2++) {
			for(size_t i3 = 0; i3 <= i; i3++) {
				if(i2 != i3) {
					mstruct[i2].replace(vargs[1][i3], mstruct[i3]);
				}
			}
		}
	}
	
	return 1;
	
}

MathStructure *find_deqn(MathStructure &mstruct);
MathStructure *find_deqn(MathStructure &mstruct) {
	if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_diff) return &mstruct;
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
	return false;
}

bool dsolve(MathStructure &m_eqn, const EvaluationOptions &eo, const MathStructure &m_diff) {
	int i_deg = m_diff[2].number().intValue();
	MathStructure m_y(m_diff[0]), m_x(m_diff[1]);
	if(m_eqn[0] == m_diff) {
		if(!m_eqn[1].contains(m_y)) {
			// y'=f(x)
			m_eqn[0] = m_y;
			for(int i = i_deg; i > 0; i--) {
				m_eqn[1].transform(STRUCT_FUNCTION);
				m_eqn[1].setFunction(CALCULATOR->f_integrate);
				m_eqn[1].addChild(m_x);
				m_eqn[1].addChild(m_undefined);
				m_eqn[1].addChild(m_undefined);
			}
			m_eqn.childrenUpdated();
			return 1;
		}
		if(i_deg == 1) {
			if(m_eqn[1] == m_y) {
				m_eqn[1] = CALCULATOR->v_e;
				m_eqn[1] ^= m_x;
				m_eqn.childrenUpdated();
				return 1;
			}
			if(m_eqn[1].isMultiplication() && m_eqn[1].size() >= 2) {
				size_t i_my = 0;
				bool b = false;
				for(size_t i = 0; i < m_eqn[1].size(); i++) {
					if(!b && m_eqn[1][i] == m_y) {
						i_my = i;
						b = true;
					} else if(m_eqn[1][i].contains(m_y)) {
						b = false;
						break;
					}
				}
				if(b) {
					// y'=a*y
					m_eqn[0] = m_y;
					MathStructure mpow(m_eqn[1]);
					mpow.delChild(i_my + 1, true);
					mpow *= m_x;
					m_eqn[1] = CALCULATOR->v_e;
					m_eqn[1] ^= mpow;
					m_eqn[1] *= "C";
					m_eqn.childrenUpdated();
					return 1;
				}
			} else if(m_eqn[1].isAddition()) {
				MathStructure m_left;
				MathStructure m_muly;
				MathStructure m_mul_exp;
				MathStructure m_exp;
				bool b = true;
					for(size_t i = 0; i < m_eqn[1].size(); i++) {
					if(m_eqn[1][i] == m_y) {
							if(m_muly.isZero()) m_muly = m_one;
						else m_muly.add(m_one, true);
					} else if(m_eqn[1][i].contains(m_y)) {
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
								} else if(m_eqn[1][i][i2].contains(m_y)) {
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
						// y' = a*y+b*y^c
						m_muly.calculateNegate(eo);
						m_exp.negate();
						m_exp += m_one;
						m_muly *= m_exp;
						if(m_muly.integrate(m_x, eo)) {
							m_eqn[0] = m_y;
							m_eqn[1] = CALCULATOR->v_e;
							m_eqn[1] ^= m_muly;
							m_mul_exp *= m_exp;
							m_eqn[1] *= m_mul_exp;
							m_eqn[1].transform(STRUCT_FUNCTION);
							m_eqn[1].setFunction(CALCULATOR->f_integrate);
							m_eqn[1].addChild(m_x);
							m_eqn[1].addChild(m_undefined);
							m_eqn[1].addChild(m_undefined);
							MathStructure m_megx = CALCULATOR->v_e;
							m_muly.negate();
							m_megx ^= m_muly;
							m_eqn[1] *= m_megx;
							m_exp.inverse();
							m_eqn[1] ^= m_exp;
							m_eqn.childrenUpdated();
							return 1;
						}
					} else if(m_left.isZero()) {
						// y'=a*y+b*y
						m_muly *= m_x;
						m_eqn[0] = m_y;
						m_eqn[1] = CALCULATOR->v_e;
						m_eqn[1] ^= m_muly;
						m_eqn[1] *= "C";
						m_eqn.childrenUpdated();
						return 1;
					} else {
						// y'=a*y+f(x)
						m_muly.calculateNegate(eo);
						if(m_muly.integrate(m_x, eo)) {
							m_eqn[0] = m_y;
							m_eqn[1] = CALCULATOR->v_e;
							m_eqn[1] ^= m_muly;
							m_eqn[1] *= m_left;
							m_eqn[1].transform(STRUCT_FUNCTION);
							m_eqn[1].setFunction(CALCULATOR->f_integrate);
							m_eqn[1].addChild(m_x);
							m_eqn[1].addChild(m_undefined);
							m_eqn[1].addChild(m_undefined);
							MathStructure m_megx = CALCULATOR->v_e;
							m_muly.negate();
							m_megx ^= m_muly;
							m_eqn[1] *= m_megx;
							m_eqn.childrenUpdated();
							return 1;
						}
					}
				}
			}
		}
	}
	if(i_deg == 1) {
		if(!m_eqn[1].contains(m_y) && !contains_ignore_diff(m_eqn[0], m_x, m_diff)) {
			if(m_eqn[0].isMultiplication() && m_eqn[0].size() >= 2) {
				size_t i_dy = 0;
				bool b = false;
				for(size_t i = 0; i < m_eqn[0].size(); i++) {
					if(!b && m_eqn[0][i] == m_diff) {
						i_dy = i;
						b = true;
					} else if(m_eqn[0][i].contains(m_diff)) {
						b = false;
						break;
					}
				}
				if(b) {
					// a*y*y'=f(x)
					m_eqn[0].delChild(i_dy + 1);
					if(m_eqn[0].integrate(m_y, eo)) {
						m_eqn[1].transform(STRUCT_FUNCTION);
						m_eqn[1].setFunction(CALCULATOR->f_integrate);
						m_eqn[1].addChild(m_x);
						m_eqn[1].addChild(m_undefined);
						m_eqn[1].addChild(m_undefined);
						m_eqn.childrenUpdated();
						m_eqn.isolate_x(eo, m_y);
						if(m_eqn[0] == m_y) {
							return 1;
						}
					}
				}
			} else if(m_eqn[0].isAddition()) {
				MathStructure m_muly;
				bool b = true;
				for(size_t i = 0; i < m_eqn[0].size(); i++) {
					if(m_eqn[0][i] == m_diff) {
						if(m_muly.isZero()) m_muly = m_one;
						else m_muly.add(m_one, true);
					} else if(m_eqn[0][i].isMultiplication() && m_eqn[0][i].size() >= 2) {
						size_t i_dy = 0;
						bool b2 = false;
						for(size_t i2 = 0; i2 < m_eqn[0][i].size(); i2++) {
							if(!b2 && m_eqn[0][i][i2] == m_diff) {
								i_dy = i2;
								b2 = true;
							} else if(m_eqn[1][i][i2].contains(m_diff)) {
								b2 = false;
								break;
							}
						}
						if(b2) {
							MathStructure m_a(m_eqn[0][i]);
							m_a.delChild(i_dy + 1);
							if(m_muly.isZero()) m_muly = m_a;
							else m_muly.add(m_a, true);
						} else {
							b = false;
							break;
						}
					} else {
						b = false;
						break;
					}
				}
				if(b && !m_muly.isZero()) {
					// a*y*y'+b*y*y'=f(x)
					if(m_muly.integrate(m_y, eo)) {
						m_eqn[0] = m_muly;
						m_eqn[1].transform(STRUCT_FUNCTION);
						m_eqn[1].setFunction(CALCULATOR->f_integrate);
						m_eqn[1].addChild(m_x);
						m_eqn[1].addChild(m_undefined);
						m_eqn[1].addChild(m_undefined);
						m_eqn.childrenUpdated();
						m_eqn.isolate_x(eo, m_y);
						if(m_eqn[0] == m_y) {
							return 1;
						}
					}
				}
			}
		}
	}
	return false;
}

DSolveFunction::DSolveFunction() : MathFunction("dsolve", 1) {}
int DSolveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure m_eqn(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.isolate_x = false;
	eo2.protected_function = CALCULATOR->f_diff;
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
	if(m_diff.size() != 3 || (!m_diff[0].isSymbolic() && !m_diff[0].isVariable()) || (!m_diff[1].isSymbolic() && !m_diff[1].isVariable()) || !m_diff[2].isInteger() || !m_diff[2].number().isPositive() || !m_diff[2].number().isLessThanOrEqualTo(10)) {
		CALCULATOR->error(true, _("No differential equation found."), NULL);
		mstruct = m_eqn; return -1;
	}
	m_eqn.isolate_x(eo2, m_diff);
	if(m_eqn.isLogicalAnd()) {
		for(size_t i = 0; i < m_eqn.size(); i++) {
			if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0] == m_diff) {
				dsolve(m_eqn[i], eo, m_diff);
			}
		}
	} else if(m_eqn.isLogicalOr()) {
		for(size_t i = 0; i < m_eqn.size(); i++) {
			if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0] == m_diff) {
				dsolve(m_eqn[i], eo, m_diff);
			} else if(m_eqn[i].isLogicalAnd()) {
				for(size_t i2 = 0; i2 < m_eqn[i].size(); i2++) {
					if(m_eqn[i][i2].isComparison() && m_eqn[i][i2].comparisonType() == COMPARISON_EQUALS && m_eqn[i][i2][0] == m_diff) {
						dsolve(m_eqn[i][i2], eo, m_diff);
					}
				}
			}
		}
	} else if(m_eqn.isComparison() && m_eqn.comparisonType() == COMPARISON_EQUALS && m_eqn[0] == m_diff) {
		dsolve(m_eqn, eo, m_diff);
	}
	if(m_eqn.contains(m_diff)) {
		m_eqn.isolate_x(eo2, m_diff[0]);
		if(m_eqn.isLogicalAnd()) {
			for(size_t i = 0; i < m_eqn.size(); i++) {
				if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0].contains(m_diff)) {
					dsolve(m_eqn[i], eo, m_diff);
				}
			}
		} else if(m_eqn.isLogicalOr()) {
			for(size_t i = 0; i < m_eqn.size(); i++) {
				if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0].contains(m_diff)) {
					dsolve(m_eqn[i], eo, m_diff);
				} else if(m_eqn[i].isLogicalAnd()) {
					for(size_t i2 = 0; i2 < m_eqn[i].size(); i2++) {
						if(m_eqn[i][i2].isComparison() && m_eqn[i][i2].comparisonType() == COMPARISON_EQUALS && m_eqn[i][i2][0].contains(m_diff)) {
							dsolve(m_eqn[i][i2], eo, m_diff);
						}
					}
				}
			}
		} else if(m_eqn.isComparison() && m_eqn.comparisonType() == COMPARISON_EQUALS && m_eqn[0].contains(m_diff)) {
			dsolve(m_eqn, eo, m_diff);
		}
	}
	if(m_eqn.contains(m_diff)) {
		CALCULATOR->error(true, _("Unable to solve differential equation."), NULL);
		mstruct = m_eqn;
		return -1;
	}
	mstruct.set(CALCULATOR->f_solve, &m_eqn, &m_diff[0], NULL);
	return 1;
}


PlotFunction::PlotFunction() : MathFunction("plot", 1, 6) {
	NumberArgument *arg = new NumberArgument();
	arg->setComplexAllowed(false);
	setArgumentDefinition(2, arg);
	setDefaultValue(2, "0");
	arg = new NumberArgument();
	arg->setComplexAllowed(false);
	setArgumentDefinition(3, arg);
	setDefaultValue(3, "10");
	setDefaultValue(4, "1001");
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(5, "x");
	setArgumentDefinition(6, new BooleanArgument());
	setDefaultValue(6, "0");
	setCondition("\\y < \\z");
}
int PlotFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	EvaluationOptions eo2;
	eo2.parse_options = eo.parse_options;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	eo2.parse_options.read_precision = DONT_READ_PRECISION;
	bool use_step_size = vargs[5].number().getBoolean();
	mstruct = vargs[0];
	eo2.calculate_functions = false;
	eo2.expand = false;
	CALCULATOR->beginTemporaryStopMessages();
	mstruct.eval(eo2);
	int im = 0;
	CALCULATOR->endTemporaryStopMessages(&im);
	if(im > 0) mstruct = vargs[0];
	eo2.calculate_functions = eo.calculate_functions;
	eo2.expand = eo.expand;
	vector<MathStructure> x_vectors, y_vectors;
	vector<PlotDataParameters*> dpds;
	if(mstruct.isMatrix() && mstruct.columns() == 2) {
		MathStructure x_vector, y_vector;
		mstruct.columnToVector(1, x_vector);
		mstruct.columnToVector(2, y_vector);
		y_vectors.push_back(y_vector);
		x_vectors.push_back(x_vector);
		PlotDataParameters *dpd = new PlotDataParameters;
		dpd->title = _("Matrix");
		dpds.push_back(dpd);
	} else if(mstruct.isVector()) {
		int matrix_index = 1, vector_index = 1;
		if(mstruct.size() > 0 && (mstruct[0].isVector() || mstruct[0].contains(vargs[4], false, true, true))) {
			for(size_t i = 0; i < mstruct.size() && !CALCULATOR->aborted(); i++) {
				MathStructure x_vector;
				if(mstruct[i].isMatrix() && mstruct[i].columns() == 2) {
					MathStructure y_vector;
					mstruct[i].columnToVector(1, x_vector);
					mstruct[i].columnToVector(2, y_vector);
					y_vectors.push_back(y_vector);
					x_vectors.push_back(x_vector);
					PlotDataParameters *dpd = new PlotDataParameters;
					dpd->title = _("Matrix");
					if(matrix_index > 1) {
						dpd->title += " ";
						dpd->title += i2s(matrix_index);
					}
					matrix_index++;
					dpds.push_back(dpd);
				} else if(mstruct[i].isVector()) {
					y_vectors.push_back(mstruct[i]);
					x_vectors.push_back(x_vector);
					PlotDataParameters *dpd = new PlotDataParameters;
					dpd->title = _("Vector");
					if(vector_index > 1) {
						dpd->title += " ";
						dpd->title += i2s(vector_index);
					}
					vector_index++;
					dpds.push_back(dpd);
				} else {
					MathStructure y_vector;
					if(use_step_size) {
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->beginTemporaryStopIntervalArithmetic();
						y_vector.set(mstruct[i].generateVector(vargs[4], vargs[1], vargs[2], vargs[3], &x_vector, eo2));
						CALCULATOR->endTemporaryStopIntervalArithmetic();
						CALCULATOR->endTemporaryStopMessages();
						if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
					} else if(!vargs[3].isInteger() || !vargs[3].representsPositive()) {
						CALCULATOR->error(true, _("Sampling rate must be a positive integer."), NULL);
					} else {
						bool overflow = false;
						int steps = vargs[3].number().intValue(&overflow);
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->beginTemporaryStopIntervalArithmetic();
						if(steps <= 1000000 && !overflow) y_vector.set(mstruct[i].generateVector(vargs[4], vargs[1], vargs[2], steps, &x_vector, eo2));
						CALCULATOR->endTemporaryStopIntervalArithmetic();
						CALCULATOR->endTemporaryStopMessages();
						if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and sampling rate."), NULL);
					}
					if(CALCULATOR->aborted()) {
						mstruct.clear();
						return 1;
					}
					if(y_vector.size() > 0) {
						x_vectors.push_back(x_vector);
						y_vectors.push_back(y_vector);
						PlotDataParameters *dpd = new PlotDataParameters;
						dpd->title = mstruct[i].print();
						dpd->test_continuous = true;
						dpds.push_back(dpd);
					}
				}
			}
		} else {
			MathStructure x_vector;
			y_vectors.push_back(mstruct);
			x_vectors.push_back(x_vector);
			PlotDataParameters *dpd = new PlotDataParameters;
			dpd->title = _("Vector");
			dpds.push_back(dpd);
		}
	} else {
		MathStructure x_vector, y_vector;
		if(use_step_size) {
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->beginTemporaryStopIntervalArithmetic();
			y_vector.set(mstruct.generateVector(vargs[4], vargs[1], vargs[2], vargs[3], &x_vector, eo2));
			CALCULATOR->endTemporaryStopIntervalArithmetic();
			CALCULATOR->endTemporaryStopMessages();
			if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
		} else if(!vargs[3].isInteger() || !vargs[3].representsPositive()) {
			CALCULATOR->error(true, _("Sampling rate must be a positive integer."), NULL);
		} else {
			bool overflow = false;
			int steps = vargs[3].number().intValue(&overflow);
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->beginTemporaryStopIntervalArithmetic();
			if(steps <= 1000000 && !overflow) y_vector.set(mstruct.generateVector(vargs[4], vargs[1], vargs[2], steps, &x_vector, eo2));
			CALCULATOR->endTemporaryStopIntervalArithmetic();
			CALCULATOR->endTemporaryStopMessages();
			if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and sampling rate."), NULL);
		}
		if(CALCULATOR->aborted()) {
			mstruct.clear();
			return 1;
		}
		if(y_vector.size() > 0) {
			x_vectors.push_back(x_vector);
			y_vectors.push_back(y_vector);
			PlotDataParameters *dpd = new PlotDataParameters;
			dpd->title = mstruct.print();
			dpd->test_continuous = true;
			dpds.push_back(dpd);
		}
	}
	if(x_vectors.size() > 0 && !CALCULATOR->aborted()) {
		PlotParameters param;
		CALCULATOR->plotVectors(&param, y_vectors, x_vectors, dpds, false, 0);
		for(size_t i = 0; i < dpds.size(); i++) {
			if(dpds[i]) delete dpds[i];
		}
	}
	mstruct.clear();
	return 1;

}

