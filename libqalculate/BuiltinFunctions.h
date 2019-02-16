
/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef BUILTIN_FUNCTIONS_H
#define BUILTIN_FUNCTIONS_H

#include <libqalculate/Function.h>
#include <libqalculate/includes.h>

#define DECLARE_BUILTIN_FUNCTION(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
					};
					
#define DECLARE_BUILTIN_FUNCTION_B(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsBoolean(const MathStructure&) const {return true;}\
					};
					
#define DECLARE_BUILTIN_FUNCTION_PI(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsInteger(const MathStructure&, bool) const {return true;}\
						bool representsPositive(const MathStructure&, bool) const {return true;}\
					};

#define DECLARE_BUILTIN_FUNCTION_RPI(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsReal(const MathStructure&, bool) const;\
						bool representsInteger(const MathStructure&, bool) const;\
						bool representsNonNegative(const MathStructure&, bool) const;\
					};															

#define DECLARE_BUILTIN_FUNCTION_R(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsPositive(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNegative(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonNegative(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonPositive(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsInteger(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNumber(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsRational(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsReal(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonComplex(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsComplex(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonZero(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsEven(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsOdd(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsUndefined(const MathStructure &vargs) const;\
					};
					
#define DECLARE_BUILTIN_FUNCTION_R2(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsNumber(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsReal(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonComplex(const MathStructure &vargs, bool allow_units = false) const;\
					};
					

#define DECLARE_BUILTIN_FUNCTION_R3(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsNumber(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsReal(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonComplex(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsComplex(const MathStructure &vargs, bool allow_units = false) const;\
						bool representsNonZero(const MathStructure &vargs, bool allow_units = false) const;\
					};
										
#define DECLARE_BUILTIN_FUNCTION_R1(x)	class x : public MathFunction { \
					  public: \
						int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);  \
						x(); \
						x(const x *function) {set(function);} \
						ExpressionItem *copy() const {return new x(this);} \
						bool representsNumber(const MathStructure &vargs, bool allow_units = false) const;\
					};															


DECLARE_BUILTIN_FUNCTION(VectorFunction)
DECLARE_BUILTIN_FUNCTION(LimitsFunction)
DECLARE_BUILTIN_FUNCTION(RankFunction)
DECLARE_BUILTIN_FUNCTION(SortFunction)
DECLARE_BUILTIN_FUNCTION(ComponentFunction)
DECLARE_BUILTIN_FUNCTION_PI(DimensionFunction)

DECLARE_BUILTIN_FUNCTION(MatrixFunction)
DECLARE_BUILTIN_FUNCTION(MergeVectorsFunction)
DECLARE_BUILTIN_FUNCTION(MatrixToVectorFunction)
DECLARE_BUILTIN_FUNCTION(AreaFunction)
DECLARE_BUILTIN_FUNCTION_PI(RowsFunction)
DECLARE_BUILTIN_FUNCTION_PI(ColumnsFunction)
DECLARE_BUILTIN_FUNCTION(RowFunction)
DECLARE_BUILTIN_FUNCTION(ColumnFunction)
DECLARE_BUILTIN_FUNCTION_PI(ElementsFunction)
DECLARE_BUILTIN_FUNCTION(ElementFunction)
DECLARE_BUILTIN_FUNCTION(TransposeFunction)
DECLARE_BUILTIN_FUNCTION(IdentityFunction)
DECLARE_BUILTIN_FUNCTION(DeterminantFunction)
DECLARE_BUILTIN_FUNCTION(PermanentFunction)
DECLARE_BUILTIN_FUNCTION(AdjointFunction)
DECLARE_BUILTIN_FUNCTION(CofactorFunction)
DECLARE_BUILTIN_FUNCTION(InverseFunction)
DECLARE_BUILTIN_FUNCTION(MagnitudeFunction)
DECLARE_BUILTIN_FUNCTION(HadamardFunction)
DECLARE_BUILTIN_FUNCTION(EntrywiseFunction)

DECLARE_BUILTIN_FUNCTION_R(FactorialFunction)
DECLARE_BUILTIN_FUNCTION_R(DoubleFactorialFunction)
DECLARE_BUILTIN_FUNCTION_R(MultiFactorialFunction)
DECLARE_BUILTIN_FUNCTION(BinomialFunction)

DECLARE_BUILTIN_FUNCTION(BitXorFunction)
DECLARE_BUILTIN_FUNCTION_B(XorFunction)
DECLARE_BUILTIN_FUNCTION(BitCmpFunction)
DECLARE_BUILTIN_FUNCTION_B(OddFunction)
DECLARE_BUILTIN_FUNCTION_B(EvenFunction)
DECLARE_BUILTIN_FUNCTION(ShiftFunction)

DECLARE_BUILTIN_FUNCTION_R(AbsFunction)
DECLARE_BUILTIN_FUNCTION(GcdFunction)
DECLARE_BUILTIN_FUNCTION(LcmFunction)
DECLARE_BUILTIN_FUNCTION_R(SignumFunction)
DECLARE_BUILTIN_FUNCTION_R(HeavisideFunction)
DECLARE_BUILTIN_FUNCTION_R(DiracFunction)
DECLARE_BUILTIN_FUNCTION_R(RoundFunction)
DECLARE_BUILTIN_FUNCTION_R(FloorFunction)
DECLARE_BUILTIN_FUNCTION_R(CeilFunction)
DECLARE_BUILTIN_FUNCTION_R(TruncFunction)
DECLARE_BUILTIN_FUNCTION(NumeratorFunction)
DECLARE_BUILTIN_FUNCTION(DenominatorFunction)
DECLARE_BUILTIN_FUNCTION(IntFunction)
DECLARE_BUILTIN_FUNCTION(FracFunction)
DECLARE_BUILTIN_FUNCTION(RemFunction)
DECLARE_BUILTIN_FUNCTION(ModFunction)

DECLARE_BUILTIN_FUNCTION(PolynomialUnitFunction)
DECLARE_BUILTIN_FUNCTION(PolynomialPrimpartFunction)
DECLARE_BUILTIN_FUNCTION(PolynomialContentFunction)
DECLARE_BUILTIN_FUNCTION(CoeffFunction)
DECLARE_BUILTIN_FUNCTION(LCoeffFunction)
DECLARE_BUILTIN_FUNCTION(TCoeffFunction)
DECLARE_BUILTIN_FUNCTION(DegreeFunction)
DECLARE_BUILTIN_FUNCTION(LDegreeFunction)

DECLARE_BUILTIN_FUNCTION_R(ReFunction)
DECLARE_BUILTIN_FUNCTION_R(ImFunction)
DECLARE_BUILTIN_FUNCTION_R2(ArgFunction)

DECLARE_BUILTIN_FUNCTION_R(IntervalFunction)
DECLARE_BUILTIN_FUNCTION(UncertaintyFunction)

DECLARE_BUILTIN_FUNCTION_R(SqrtFunction)
DECLARE_BUILTIN_FUNCTION(CbrtFunction)
DECLARE_BUILTIN_FUNCTION_R(RootFunction)
DECLARE_BUILTIN_FUNCTION(SquareFunction)

DECLARE_BUILTIN_FUNCTION(ExpFunction)

DECLARE_BUILTIN_FUNCTION_R(LogFunction)
DECLARE_BUILTIN_FUNCTION(LognFunction)

DECLARE_BUILTIN_FUNCTION_R3(LambertWFunction)

DECLARE_BUILTIN_FUNCTION_R2(SinFunction)
DECLARE_BUILTIN_FUNCTION_R2(CosFunction)
DECLARE_BUILTIN_FUNCTION_R2(TanFunction)
DECLARE_BUILTIN_FUNCTION_R1(AsinFunction)
DECLARE_BUILTIN_FUNCTION_R1(AcosFunction)
DECLARE_BUILTIN_FUNCTION_R2(AtanFunction)
DECLARE_BUILTIN_FUNCTION_R2(SinhFunction)
DECLARE_BUILTIN_FUNCTION_R2(CoshFunction)
DECLARE_BUILTIN_FUNCTION_R2(TanhFunction)
DECLARE_BUILTIN_FUNCTION_R2(AsinhFunction)
DECLARE_BUILTIN_FUNCTION_R1(AcoshFunction)
DECLARE_BUILTIN_FUNCTION(AtanhFunction)
DECLARE_BUILTIN_FUNCTION_R1(Atan2Function)
DECLARE_BUILTIN_FUNCTION(RadiansToDefaultAngleUnitFunction)
DECLARE_BUILTIN_FUNCTION_R2(SincFunction)

DECLARE_BUILTIN_FUNCTION(ZetaFunction)
DECLARE_BUILTIN_FUNCTION(GammaFunction)
DECLARE_BUILTIN_FUNCTION(DigammaFunction)
DECLARE_BUILTIN_FUNCTION(BetaFunction)
DECLARE_BUILTIN_FUNCTION(AiryFunction)
DECLARE_BUILTIN_FUNCTION(BesseljFunction)
DECLARE_BUILTIN_FUNCTION(BesselyFunction)
DECLARE_BUILTIN_FUNCTION(ErfFunction)
DECLARE_BUILTIN_FUNCTION(ErfcFunction)

DECLARE_BUILTIN_FUNCTION(TotalFunction)
DECLARE_BUILTIN_FUNCTION(PercentileFunction)
DECLARE_BUILTIN_FUNCTION(MinFunction)
DECLARE_BUILTIN_FUNCTION(MaxFunction)
DECLARE_BUILTIN_FUNCTION(ModeFunction)
DECLARE_BUILTIN_FUNCTION_RPI(RandFunction)

DECLARE_BUILTIN_FUNCTION(DateFunction)
DECLARE_BUILTIN_FUNCTION(DateTimeFunction)
DECLARE_BUILTIN_FUNCTION(TimeValueFunction)
DECLARE_BUILTIN_FUNCTION(TimestampFunction)
DECLARE_BUILTIN_FUNCTION(TimestampToDateFunction)
DECLARE_BUILTIN_FUNCTION(DaysFunction)
DECLARE_BUILTIN_FUNCTION(YearFracFunction)
DECLARE_BUILTIN_FUNCTION(WeekFunction)
DECLARE_BUILTIN_FUNCTION(WeekdayFunction)
DECLARE_BUILTIN_FUNCTION(MonthFunction)
DECLARE_BUILTIN_FUNCTION(DayFunction)
DECLARE_BUILTIN_FUNCTION(YearFunction)
DECLARE_BUILTIN_FUNCTION(YeardayFunction)
DECLARE_BUILTIN_FUNCTION(TimeFunction)
DECLARE_BUILTIN_FUNCTION(AddDaysFunction)
DECLARE_BUILTIN_FUNCTION(AddMonthsFunction)
DECLARE_BUILTIN_FUNCTION(AddYearsFunction)

DECLARE_BUILTIN_FUNCTION(LunarPhaseFunction)
DECLARE_BUILTIN_FUNCTION(NextLunarPhaseFunction)

DECLARE_BUILTIN_FUNCTION(BinFunction)
DECLARE_BUILTIN_FUNCTION(OctFunction)
DECLARE_BUILTIN_FUNCTION(HexFunction)
DECLARE_BUILTIN_FUNCTION(BaseFunction)
DECLARE_BUILTIN_FUNCTION(RomanFunction)

DECLARE_BUILTIN_FUNCTION_PI(AsciiFunction)
DECLARE_BUILTIN_FUNCTION(CharFunction)

DECLARE_BUILTIN_FUNCTION_PI(LengthFunction)
DECLARE_BUILTIN_FUNCTION(ConcatenateFunction)

DECLARE_BUILTIN_FUNCTION(ReplaceFunction)
DECLARE_BUILTIN_FUNCTION(StripUnitsFunction)

DECLARE_BUILTIN_FUNCTION(GenerateVectorFunction)
DECLARE_BUILTIN_FUNCTION(ForFunction)
DECLARE_BUILTIN_FUNCTION(SumFunction)
DECLARE_BUILTIN_FUNCTION(ProductFunction)
DECLARE_BUILTIN_FUNCTION(ProcessFunction)
DECLARE_BUILTIN_FUNCTION(ProcessMatrixFunction)
DECLARE_BUILTIN_FUNCTION(CustomSumFunction)
DECLARE_BUILTIN_FUNCTION(FunctionFunction)
DECLARE_BUILTIN_FUNCTION(SelectFunction)
DECLARE_BUILTIN_FUNCTION(TitleFunction)
DECLARE_BUILTIN_FUNCTION(IFFunction)
DECLARE_BUILTIN_FUNCTION(IsNumberFunction)
DECLARE_BUILTIN_FUNCTION(IsRealFunction)
DECLARE_BUILTIN_FUNCTION(IsRationalFunction)
DECLARE_BUILTIN_FUNCTION(IsIntegerFunction)
DECLARE_BUILTIN_FUNCTION(RepresentsNumberFunction)
DECLARE_BUILTIN_FUNCTION(RepresentsRealFunction)
DECLARE_BUILTIN_FUNCTION(RepresentsRationalFunction)
DECLARE_BUILTIN_FUNCTION(RepresentsIntegerFunction)
DECLARE_BUILTIN_FUNCTION(ErrorFunction)
DECLARE_BUILTIN_FUNCTION(WarningFunction)
DECLARE_BUILTIN_FUNCTION(MessageFunction)
DECLARE_BUILTIN_FUNCTION(SaveFunction)
DECLARE_BUILTIN_FUNCTION(LoadFunction)
DECLARE_BUILTIN_FUNCTION(ExportFunction)

DECLARE_BUILTIN_FUNCTION(RegisterFunction)
DECLARE_BUILTIN_FUNCTION(StackFunction)

DECLARE_BUILTIN_FUNCTION(DeriveFunction)
DECLARE_BUILTIN_FUNCTION(IntegrateFunction)
DECLARE_BUILTIN_FUNCTION(SolveFunction)
DECLARE_BUILTIN_FUNCTION(SolveMultipleFunction)
DECLARE_BUILTIN_FUNCTION(DSolveFunction)
DECLARE_BUILTIN_FUNCTION(LimitFunction)

DECLARE_BUILTIN_FUNCTION_R2(liFunction)
DECLARE_BUILTIN_FUNCTION_R2(LiFunction)
DECLARE_BUILTIN_FUNCTION_R2(EiFunction)
DECLARE_BUILTIN_FUNCTION_R2(SiFunction)
DECLARE_BUILTIN_FUNCTION_R2(CiFunction)
DECLARE_BUILTIN_FUNCTION_R2(ShiFunction)
DECLARE_BUILTIN_FUNCTION_R2(ChiFunction)
DECLARE_BUILTIN_FUNCTION_R2(IGammaFunction)
DECLARE_BUILTIN_FUNCTION_R2(GammaIncFunction)

DECLARE_BUILTIN_FUNCTION(PlotFunction)

#endif
