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
#include <limits.h>
#include <limits>
#include <math.h>
#include <algorithm>

#include "MathStructure-support.h"

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
	using std::unordered_map;
#elif 	defined(__GNUC__)

#	ifndef __has_include
#	define __has_include(x) 0
#	endif

#	if (defined(__clang__) && __has_include(<tr1/unordered_map>)) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#		include <tr1/unordered_map>
		namespace Sgi = std;
#		define unordered_map std::tr1::unordered_map
#	else
#		if __GNUC__ < 3
#			include <hash_map.h>
			namespace Sgi { using ::hash_map; }; // inherit globals
#		else
#			include <ext/hash_map>
#			if __GNUC__ == 3 && __GNUC_MINOR__ == 0
				namespace Sgi = std;               // GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
#			endif
#		endif
#		define unordered_map Sgi::hash_map
#	endif
#else      // ...  there are other compilers, right?
	namespace Sgi = std;
#	define unordered_map Sgi::hash_map
#endif

using std::string;
using std::cout;
using std::vector;
using std::endl;

#define FR_FUNCTION(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2b(FUNC)	if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity()))
#define FR_FUNCTION_2c0 	{return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2c1 	{return -1;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); FR_FUNCTION_2b(FUNC) FR_FUNCTION_2c0

#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_TEST(i)			NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(i)		NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define RATIONAL_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_rational##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_rational##i->setRationalNumber(true); setArgumentDefinition(i, arg_rational##i);
#define RATIONAL_POLYNOMIAL_ARGUMENT(i)				Argument *arg_poly##i = new Argument(); arg_poly##i->setRationalPolynomial(true); setArgumentDefinition(i, arg_poly##i);
#define RATIONAL_POLYNOMIAL_ARGUMENT_HV(i)			Argument *arg_poly##i = new Argument(); arg_poly##i->setRationalPolynomial(true); arg_poly##i->setHandleVector(true); setArgumentDefinition(i, arg_poly##i);

OddFunction::OddFunction() : MathFunction("odd", 1) {
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int OddFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].representsOdd()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsEven()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
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
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int EvenFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsOdd()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(mstruct.representsOdd()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}

bool represents_imaginary(const MathStructure &m, bool allow_units = true) {
	switch(m.type()) {
		case STRUCT_NUMBER: {return m.number().hasImaginaryPart() && !m.number().hasRealPart();}
		case STRUCT_VARIABLE: {return m.variable()->isKnown() && represents_imaginary(((KnownVariable*) m.variable())->get(), allow_units);}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < m.size(); i++) {
				if(!represents_imaginary(m[i])) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool c = false;
			for(size_t i = 0; i < m.size(); i++) {
				if(represents_imaginary(m[i])) {
					c = !c;
				} else if(!m[i].representsReal(allow_units)) {
					return false;
				}
			}
			return c;
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_POWER: {return (m[1].isNumber() && m[1].number().denominatorIsTwo() && m[0].representsNegative()) || (represents_imaginary(m[0]) && m[1].representsOdd());}
		default: {return false;}
	}
}

AbsFunction::AbsFunction() : MathFunction("abs", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool AbsFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsInteger(allow_units);}
bool AbsFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsRational(allow_units);}
bool AbsFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool AbsFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsEven(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsEven(allow_units);}
bool AbsFunction::representsOdd(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsOdd(allow_units);}
bool AbsFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int AbsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		if(eo.approximation != APPROXIMATION_APPROXIMATE && mstruct.number().hasImaginaryPart() && mstruct.number().hasRealPart()) {
			MathStructure m_i(mstruct.number().imaginaryPart());
			m_i ^= nr_two;
			mstruct.number().clearImaginary();
			mstruct.numberUpdated();
			mstruct ^= nr_two;
			mstruct += m_i;
			mstruct ^= nr_half;
			return 1;
		}
		Number nr(mstruct.number());
		if(!nr.abs() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			return -1;
		}
		mstruct = nr;
		return 1;
	}
	if(mstruct.isPower() && mstruct[0].representsPositive()) {
		if(mstruct[1].isNumber() && !mstruct[1].number().hasRealPart()) {
			mstruct.set(1, 1, 0, true);
			return 1;
		} else if(mstruct[1].isMultiplication() && mstruct.size() > 0 && mstruct[1][0].isNumber() && !mstruct[1][0].number().hasRealPart()) {
			bool b = true;
			for(size_t i = 1; i < mstruct[1].size(); i++) {
				if(!mstruct[1][i].representsNonComplex()) {b = false; break;}
			}
			if(b) {
				mstruct.set(1, 1, 0, true);
				return 1;
			}
		}
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
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2) {
		mstruct[0].transform(this);
		mstruct.childUpdated(1);
		return 1;
	}
	if(mstruct.isPower() && mstruct[1].representsReal()) {
		mstruct[0].transform(this);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
			return 1;
		} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
			mstruct.negate();
			return 1;
		}
	}
	MathStructure m_im(CALCULATOR->getFunctionById(FUNCTION_ID_IM), &mstruct, NULL);
	CALCULATOR->beginTemporaryStopMessages();
	m_im.eval(eo);
	if(!m_im.containsFunctionId(FUNCTION_ID_IM) && m_im.representsReal(true)) {
		MathStructure m_re(CALCULATOR->getFunctionById(FUNCTION_ID_RE), &mstruct, NULL);
		m_re.eval(eo);
		if(!m_re.containsFunctionId(FUNCTION_ID_RE)) {
			if(m_re.isZero()) {
				mstruct = m_im;
				mstruct.transform(this);
				CALCULATOR->endTemporaryStopMessages(true);
				return 1;
			} else if(!m_im.isZero()) {
				m_re ^= nr_two;
				m_im ^= nr_two;
				mstruct = m_re;
				mstruct += m_im;
				mstruct ^= nr_half;
				CALCULATOR->endTemporaryStopMessages(true);
				return 1;
			}
		}
	}
	CALCULATOR->endTemporaryStopMessages();
	if(mstruct.isAddition()) {
		MathStructure factor_mstruct(1, 1, 0);
		MathStructure mnew;
		if(factorize_find_multiplier(mstruct, mnew, factor_mstruct) && !factor_mstruct.isZero() && !mnew.isZero()) {
			factor_mstruct.transform(this);
			mstruct = mnew;
			mstruct.transform(this);
			mstruct.multiply(factor_mstruct);
			return 1;
		}
	}
	return -1;
}
GcdFunction::GcdFunction() : MathFunction("gcd", 2, -1) {
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(1)
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(2)
	RATIONAL_POLYNOMIAL_ARGUMENT(3)
}
int GcdFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	bool b_number = true;
	for(size_t i = 0; i < vargs.size(); i++) {
		if(!vargs[i].isNumber()) {
			b_number = false;
			break;
		}
	}
	if(b_number) {
		for(size_t i = 1; i < vargs.size(); i++) {
			if(!mstruct.number().gcd(vargs[i].number())) return 0;
		}
		mstruct.numberUpdated();
	} else {
		MathStructure m1;
		for(size_t i = 1; i < vargs.size(); i++) {
			m1 = mstruct;
			if(!MathStructure::gcd(m1, vargs[i], mstruct, eo)) return 0;
		}
	}
	return 1;
}
LcmFunction::LcmFunction() : MathFunction("lcm", 2, -1) {
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(1)
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(2)
	RATIONAL_POLYNOMIAL_ARGUMENT(3)
}
int LcmFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	bool b_number = true;
	for(size_t i = 0; i < vargs.size(); i++) {
		if(!vargs[i].isNumber()) {
			b_number = false;
			break;
		}
	}
	if(b_number) {
		for(size_t i = 1; i < vargs.size(); i++) {
			if(!mstruct.number().lcm(vargs[i].number())) return 0;
		}
		mstruct.numberUpdated();
	} else {
		MathStructure m1;
		for(size_t i = 1; i < vargs.size(); i++) {
			m1 = mstruct;
			if(!MathStructure::lcm(m1, vargs[i], mstruct, eo)) return 0;
		}
	}
	return 1;
}
bool divisors_combine(MathStructure &m, vector<Number> factors, size_t skip, size_t pos = 0, Number nr = nr_one) {
	for(; pos < factors.size() - skip; pos++) {
		if(CALCULATOR->aborted()) return false;
		if(skip > 0) {
			if(!divisors_combine(m, factors, skip - 1, pos + 1, nr)) return false;
		}
		nr *= factors[pos];
	}
	bool b = false;
	for(size_t i = m.size(); i > 0; i--) {
		if(nr >= m[i - 1].number()) {
			if(nr != m[i - 1].number()) m.insertChild(nr, i + 1);
			b = true;
			break;
		}
	}
	if(!b) m.insertChild(nr, 1);
	return true;
}
DivisorsFunction::DivisorsFunction() : MathFunction("divisors", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO));
}
int DivisorsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vector<Number> factors;
	Number nr(vargs[0].number());
	nr.abs();
	mstruct.clearVector();
	if(nr.isOne()) {
		mstruct.addChild(nr);
		return 1;
	}
	if(!nr.factorize(factors)) return 0;
	if(nr.isLessThan(Number(1, 1, factors.size() / 2.5))) {
		bool b = false;
		long int n = vargs[0].number().lintValue(&b);
		if(!b) {
			if(n < 0) n = -n;
			mstruct.clearVector();
			mstruct.addChild(m_one);
			long int i_last = n / factors[0].lintValue();
			for(long int i = 2; i <= i_last; i++) {
				if(CALCULATOR->aborted()) return 0;
				if(n % i == 0) mstruct.addChild(MathStructure(i, 1L, 0L));
			}
			mstruct.addChild(MathStructure(n, 1L, 0L));
			return 1;
		}
	}
	if(factors.size() == 2) {
		mstruct.addChild(factors[0]);
		if(factors[0] != factors[1]) mstruct.addChild(factors[1]);
	} else if(factors.size() > 2) {
		for(size_t i = factors.size() - 1; i > 0; i--) {
			if(!divisors_combine(mstruct, factors, i)) return 0;
		}
	}
	mstruct.insertChild(m_one, 1);
	mstruct.addChild(nr);
	return 1;
}

#include "primes.h"

PrimesFunction::PrimesFunction() : MathFunction("primes", 1) {
	NumberArgument *iarg = new NumberArgument();
	iarg->setMin(&nr_one);
	Number nmax(PRIMES_L[NR_OF_PRIMES_L - 1]);
	iarg->setMax(&nmax);
	iarg->setHandleVector(false);
	setArgumentDefinition(1, iarg);
}
int PrimesFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	nr.floor();
	if(!nr.isInteger()) return 0;
	mstruct.clearVector();
	long int v = nr.intValue();
	for(size_t i = 0; i < NR_OF_PRIMES_L; i++) {
		if(PRIMES_L[i] > v) break;
		mstruct.addChild_nocopy(new MathStructure(PRIMES_L[i], 1L, 0L));
	}
	return 1;
}
IsPrimeFunction::IsPrimeFunction() : MathFunction("isprime", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
}
int IsPrimeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr;
	int r = mpz_probab_prime_p(mpq_numref(vargs[0].number().internalRational()), 25);
	if(r) mstruct = m_one;
	else mstruct = m_zero;
	if(r == 1) CALCULATOR->error(false, _("The value is probably a prime number, but it is not certain."), NULL);
	return 1;
}
NthPrimeFunction::NthPrimeFunction() : MathFunction("nthprime", 1) {
	IntegerArgument *iarg = new IntegerArgument();
	iarg->setMin(&nr_one);
	Number nmax(PRIME_M_COUNT, 1, 5);
	iarg->setMax(&nmax);
	setArgumentDefinition(1, iarg);
}
int NthPrimeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number() <= NR_OF_PRIMES_L) {
		mstruct.set(PRIMES_L[vargs[0].number().lintValue() - 1], 1L, 0L);
		return 1;
	}
	Number l10(vargs[0].number());
	l10.divide(100000L);
	l10.floor();
	if(l10 <= PRIME_M_COUNT) {
		Number n(l10.lintValue(), 1, 5);
		mpz_t i;
		mpz_init(i);
		if(PRIME_M[l10.lintValue() - 1] > LONG_MAX) {
			mpz_set_si(i, (long int) (PRIME_M[l10.lintValue() - 1] / ULONG_MAX));
			mpz_mul_si(i, i, LONG_MAX);
			mpz_add_ui(i, i, (unsigned long int) (PRIME_M[l10.lintValue() - 1] % LONG_MAX));
		} else {
			mpz_set_si(i, (long int) PRIME_M[l10.lintValue() - 1]);
		}
		while(n < vargs[0].number()) {
			if(CALCULATOR->aborted()) return 0;
			n++;
			mpz_nextprime(i, i);
		}
		Number nr;
		nr.setInternal(i);
		mstruct = nr;
		mpz_clear(i);
		return 1;
	}
	return 0;
}

NextPrimeFunction::NextPrimeFunction() : MathFunction("nextprime", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
}
int NextPrimeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	nr.ceil();
	if(!nr.isInteger()) return 0;
	if(nr <= 2) {
		mstruct = nr_two;
		return 1;
	}
	if(nr <= PRIMES_L[NR_OF_PRIMES_L - 1]) {
		long int prime_i = NR_OF_PRIMES_L;
		long int step = prime_i / 2;
		while(nr != PRIMES_L[prime_i - 1]) {
			if(nr < PRIMES_L[prime_i - 1]) {
				prime_i -= step;
			} else {
				prime_i += step;
				if(step == 1 && nr < PRIMES_L[prime_i - 1]) break;
			}
			if(step != 1) step /= 2;
		}
		mstruct.set(PRIMES_L[prime_i - 1], 1L, 0L);
		return 1;
	}
	mpz_t i;
	mpz_init(i);
	mpz_sub_ui(i, mpq_numref(nr.internalRational()), 1);
	mpz_nextprime(i, i);
	if(mpz_sizeinbase(i, 2) > 40) {
		int r = mpz_probab_prime_p(i, 25);
		while(!r) {mpz_nextprime(i, i); r = mpz_probab_prime_p(i, 25);}
		if(r == 1) CALCULATOR->error(false, _("The returned value is probably a prime number, but it is not completely certain."), NULL);
	}
	nr.setInternal(i);
	mstruct = nr;
	mpz_clear(i);
	return 1;
}
PrevPrimeFunction::PrevPrimeFunction() : MathFunction("prevprime", 1) {
	NumberArgument *iarg = new NumberArgument();
	iarg->setMin(&nr_two);
	setArgumentDefinition(1, iarg);
}
int PrevPrimeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	nr.floor();
	if(!nr.isInteger()) return 0;
	if(nr.isTwo()) {
		mstruct = nr_two;
		return 1;
	}
	if(nr <= PRIMES_L[NR_OF_PRIMES_L - 1]) {
		long int prime_i = NR_OF_PRIMES_L;
		long int step = prime_i / 2;
		while(nr != PRIMES_L[prime_i - 1]) {
			if(nr < PRIMES_L[prime_i - 1]) {
				prime_i -= step;
				if(step == 1 && nr > PRIMES_L[prime_i - 1]) break;
			} else {
				prime_i += step;
			}
			if(step != 1) step /= 2;
		}
		mstruct.set(PRIMES_L[prime_i - 1], 1L, 0L);
		return 1;
	}
	mpz_t i, p;
	mpz_inits(i, p, NULL);
	mpz_sub_ui(i, mpq_numref(nr.internalRational()), 1);
	mpz_nextprime(p, i);
	while(mpz_cmp(p, mpq_numref(nr.internalRational())) > 0) {
		if(CALCULATOR->aborted()) {
			mpz_clears(i, p);
			return 0;
		}
		mpz_sub_ui(i, i, 1);
		mpz_nextprime(p, i);
	}
	if(mpz_sizeinbase(p, 2) > 40) {
		int r = mpz_probab_prime_p(p, 25);
		while(!r) {mpz_sub_ui(i, i, 1); mpz_nextprime(p, i); r = mpz_probab_prime_p(p, 25);}
		if(r == 1) CALCULATOR->error(false, _("The returned value is probably a prime number, but it is not completely certain."), NULL);
	}
	nr.setInternal(p);
	mstruct = nr;
	mpz_clears(i, p, NULL);
	return 1;
}

unordered_map<long long int, unordered_map<long long int, long long int>> primecount_cache;

long long int primecount_phi(long long int x, long long int a) {
	unordered_map<long long int, unordered_map<long long int, long long int>>::iterator it = primecount_cache.find(x);
	if(it != primecount_cache.end()) {
		unordered_map<long long int, long long int>::iterator it2 = it->second.find(a);
		if(it2 != it->second.end()) return it2->second;
	}
	if(a == 1) {
		return (x + 1) / 2;
	}
	long long int v = primecount_phi(x, a - 1) - primecount_phi(x / PRIMES_L[a - 1], a - 1);
	primecount_cache[x][a] = v;
	return v;
}

long long int primecount(long long int x) {
	if(x == 2) return 1;
	if(x < 2) return 0;
	if(x <= PRIMES_L[NR_OF_PRIMES_L - 1]) {
		long int prime_i = NR_OF_PRIMES_L;
		long int step = prime_i / 2;
		while(x != PRIMES_L[prime_i - 1]) {
			if(x < PRIMES_L[prime_i - 1]) {
				prime_i -= step;
				if(step == 1 && x > PRIMES_L[prime_i - 1]) break;
			} else {
				prime_i += step;
			}
			if(step != 1) step /= 2;
		}
		return prime_i;
	}
	if(CALCULATOR->aborted()) return 0;
	long long int a = primecount(::sqrt(sqrt(x)));
	long long int b = primecount(::sqrt(x));
	long long int c = primecount(::cbrt(x));
	long long int sum = primecount_phi(x, a) + ((b + a - 2) * (b - a + 1) / 2);
	for(long int i = a + 1; i <= b; i++) {
		if(CALCULATOR->aborted()) return 0;
		long long int w = x / PRIMES_L[i - 1];
		long long int lim = primecount(::sqrt(w));
		sum -= primecount(w);
		if(i <= c) {
			for(long long int i2 = i; i2 <= lim; i2++) {
				if(CALCULATOR->aborted()) return 0;
				sum -= primecount(w / PRIMES_L[i2 - 1]) - i2 + 1;
			}
		}
	}
	return sum;
}

PrimeCountFunction::PrimeCountFunction() : MathFunction("primePi", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
}
int PrimeCountFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	nr.floor();
	if(!nr.isInteger()) return 0;
	if(nr.integerLength() < 41) {
		long long int v = primecount(nr.llintValue());
		if(CALCULATOR->aborted()) return 0;
		if(v > LONG_MAX) {
			nr.set(v / LONG_MAX);
			nr *= LONG_MAX;
			nr += (v % LONG_MAX);
			mstruct = nr;
		} else {
			mstruct = Number((long int) v, 1L, 0L);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT) return 0;
	// Approximation (lower endpoint requires x >= 88789)
	Number nlog(nr);
	if(nlog.ln()) {
		Number nlog2(nlog);
		Number nlog3(nlog);
		if(nlog2.square() && nlog3.raise(nr_three) && nlog.recip() && nlog2.recip() && nlog3.recip() && nlog2.multiply(2) && nlog3.multiply(Number(759, 100))) {
			Number lower(1, 1);
			if(lower.add(nlog) && lower.add(nlog2)) {
				Number upper(lower);
				if(upper.add(nlog3) && lower.multiply(nr) && upper.multiply(nr) && lower.multiply(nlog) && upper.multiply(nlog)) {
					nr.setInterval(lower, upper, false);
					mstruct = nr;
					return 1;
				}
			}
		}
	}
	return 1;
}

SignumFunction::SignumFunction() : MathFunction("sgn", 1, 2) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setDefaultValue(2, "0");
}
bool SignumFunction::representsPositive(const MathStructure&, bool allow_units) const {return false;}
bool SignumFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonNegative(true);}
bool SignumFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonPositive(true);}
bool SignumFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNumber(true);}
bool SignumFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonComplex(true);}
bool SignumFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsComplex(true);}
bool SignumFunction::representsNonZero(const MathStructure &vargs, bool) const {return (vargs.size() == 2 && !vargs[1].isZero()) || (vargs.size() >= 1 && vargs[0].representsNonZero(true));}
bool SignumFunction::representsEven(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsOdd(const MathStructure &vargs, bool b) const {return representsNonZero(vargs, b);}
bool SignumFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() >= 1 && vargs[0].representsUndefined();}
int SignumFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber() && (vargs.size() == 1 || vargs[1].isZero())) {
		Number nr(mstruct.number());
		if(!nr.signum() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			if(mstruct.number().isNonZero()) {
				MathStructure *mabs = new MathStructure(mstruct);
				mabs->transformById(FUNCTION_ID_ABS);
				mstruct.divide_nocopy(mabs);
				return 1;
			}
			return -1;
		} else {
			mstruct = nr;
			return 1;
		}
	}
	if((vargs.size() > 1 && vargs[1].isOne() && mstruct.representsNonNegative(true)) || mstruct.representsPositive(true)) {
		mstruct.set(1, 1, 0);
		return 1;
	}
	if((vargs.size() > 1 && vargs[1].isMinusOne() && mstruct.representsNonPositive(true)) || mstruct.representsNegative(true)) {
		mstruct.set(-1, 1, 0);
		return 1;
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(vargs.size() > 1) mstruct[i].transform(STRUCT_FUNCTION, vargs[1]);
			else mstruct[i].transform(STRUCT_FUNCTION);
			mstruct[i].setFunction(this);

		}
		mstruct.childrenUpdated();
		return 1;
	}
	if(vargs.size() > 1 && mstruct.isZero()) {
		mstruct.set(vargs[1], true);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(cr == COMPARISON_RESULT_LESS || (vargs.size() > 1 && vargs[1].isOne() && COMPARISON_IS_EQUAL_OR_LESS(cr))) {
			mstruct.set(1, 1, 0);
			return 1;
		} else if(cr == COMPARISON_RESULT_GREATER || (vargs.size() > 1 && vargs[1].isMinusOne() && COMPARISON_IS_EQUAL_OR_GREATER(cr))) {
			mstruct.set(-1, 1, 0);
			return 1;
		}
	}
	return -1;
}

CeilFunction::CeilFunction() : MathFunction("ceil", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int CeilFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].isNumber()) {
		FR_FUNCTION(ceil)
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.ceil() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate())) {
			mstruct.set(nr);
			return 1;
		}
	} else if(!mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT && !vargs[0].isApproximate()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		MathStructure mstruct2(mstruct);
		mstruct2.eval(eo2);
		if(mstruct2.isNumber()) {
			Number nr(mstruct2.number());
			if(nr.ceil() && !nr.isApproximate()) {
				mstruct.set(nr);
				return 1;
			}
		}
	}
	if(mstruct.representsInteger(false)) return 1;
	return -1;
}
bool CeilFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool CeilFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool CeilFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool CeilFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsEven();}
bool CeilFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsOdd();}
bool CeilFunction::representsUndefined(const MathStructure&) const {return false;}

FloorFunction::FloorFunction() : MathFunction("floor", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int FloorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].isNumber()) {
		FR_FUNCTION(floor)
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.floor() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate())) {
			mstruct.set(nr);
			return 1;
		}
	} else if(!mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT && !vargs[0].isApproximate()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		MathStructure mstruct2(mstruct);
		mstruct2.eval(eo2);
		if(mstruct2.isNumber()) {
			Number nr(mstruct2.number());
			if(nr.floor() && !nr.isApproximate()) {
				mstruct.set(nr);
				return 1;
			}
		}
	}
	if(mstruct.representsInteger(false)) return 1;
	return -1;
}
bool FloorFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool FloorFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool FloorFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool FloorFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsEven();}
bool FloorFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsOdd();}
bool FloorFunction::representsUndefined(const MathStructure&) const {return false;}

TruncFunction::TruncFunction() : MathFunction("trunc", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int TruncFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].isNumber()) {
		FR_FUNCTION(trunc)
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.trunc() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate())) {
			mstruct.set(nr);
			return 1;
		}
	} else if(!mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT && !vargs[0].isApproximate()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		MathStructure mstruct2(mstruct);
		mstruct2.eval(eo2);
		if(mstruct2.isNumber()) {
			Number nr(mstruct2.number());
			if(nr.trunc() && !nr.isApproximate()) {
				mstruct.set(nr);
				return 1;
			}
		}
	}
	if(mstruct.representsInteger(false)) return 1;
	return -1;
}
bool TruncFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool TruncFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool TruncFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool TruncFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsEven();}
bool TruncFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsOdd();}
bool TruncFunction::representsUndefined(const MathStructure&) const {return false;}

RoundFunction::RoundFunction() : MathFunction("round", 1, 3) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setArgumentDefinition(2, new IntegerArgument());
	setDefaultValue(2, "0");
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "0");
}
int RoundFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].isNumber()) {
		Number nr(vargs[0].number());
		if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(vargs[1].number());
		if(nr.round(vargs.size() >= 3 ? vargs[2].number().getBoolean() : true) && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate())) {
			if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(-vargs[1].number());
			mstruct.set(nr);
			return 1;
		}
		return 0;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(vargs[1].number());
		if(nr.round(vargs.size() >= 3 ? vargs[2].number().getBoolean() : true) && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate())) {
			if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(-vargs[1].number());
			mstruct.set(nr);
			return 1;
		}
	} else if(!mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT && !vargs[0].isApproximate()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		MathStructure mstruct2(mstruct);
		mstruct2.eval(eo2);
		if(mstruct2.isNumber()) {
			Number nr(mstruct2.number());
			if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(vargs[1].number());
			if(nr.round(vargs.size() >= 3 ? vargs[2].number().getBoolean() : true) && !nr.isApproximate()) {
				if(vargs.size() >= 2 && !vargs[1].isZero()) nr.exp10(-vargs[1].number());
				mstruct.set(nr);
				return 1;
			}
		}
	}
	if(mstruct.representsInteger(false)) return 1;
	return -1;
}
bool RoundFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool RoundFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool RoundFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && (vargs.size() < 2 || vargs[1].representsNonPositive());}
bool RoundFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool RoundFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsEven() && (vargs.size() < 2 || vargs[1].representsNonPositive());}
bool RoundFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsOdd() && (vargs.size() < 2 || vargs[1].representsNonPositive());}
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
	arg_rational_1->setHandleVector(true);
	setArgumentDefinition(1, arg_rational_1);
}
int NumeratorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
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
	if(mstruct.isVector()) return -1;
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
	NON_COMPLEX_NUMBER_ARGUMENT_NO_TEST(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
bool powmod(Number &nr, const Number &base, const Number &exp, const Number &div, bool b_rem) {
	mpz_t i;
	mpz_init(i);
	if(exp.isNegative()) {
		mpz_gcd(i, mpq_numref(base.internalRational()), mpq_numref(div.internalRational()));
		if(mpz_cmp_ui(i, 1) != 0) {
			mpz_clear(i);
			return false;
		}
	}
	mpz_powm(i, mpq_numref(base.internalRational()), mpq_numref(exp.internalRational()), mpq_numref(div.internalRational()));
	nr.setInternal(i);
	if(b_rem && base.isNegative() && exp.isOdd()) nr -= div;
	mpz_clear(i);
	return true;
}
PowerModFunction::PowerModFunction() : MathFunction("powmod", 3) {
	setArgumentDefinition(1, new IntegerArgument(""));
	setArgumentDefinition(2, new IntegerArgument(""));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO));
}
int PowerModFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct.clear();
	if(!powmod(mstruct.number(), vargs[0].number(), vargs[1].number(), vargs[2].number(), false)) return 0;
	return 1;
}
void remove_overflow_message() {
	vector<CalculatorMessage> message_vector;
	CALCULATOR->endTemporaryStopMessages(false, &message_vector);
	for(size_t i = 0; i < message_vector.size();) {
		if(message_vector[i].message() == _("Floating point overflow")) message_vector.erase(message_vector.begin() + i);
		else i++;
	}
	if(!message_vector.empty()) CALCULATOR->addMessages(&message_vector);
}
int RemFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(!vargs[1].isInteger()) {
		if(!vargs[0].isNumber()) {
			mstruct = vargs[0];
			mstruct.eval(eo);
			if(!mstruct.isNumber() || !mstruct.number().isReal()) return -1;
			Number nr(mstruct.number());
			FR_FUNCTION_2b(rem)
			FR_FUNCTION_2c1
		}
	} else if(!vargs[0].isNumber()) {
		if(vargs[0].isPower() && vargs[0][0].isInteger() && vargs[0][1].isInteger() && vargs[0][1].number().isPositive() && !vargs[0][0].number().isZero()) {
			Number nr;
			if(powmod(nr, vargs[0][0].number(), vargs[0][1].number(), vargs[1].number(), true)) {mstruct = nr; return 1;}
		}
		mstruct = vargs[0];
		CALCULATOR->beginTemporaryStopMessages();
		if(eo.approximation != APPROXIMATION_EXACT) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			mstruct.eval(eo2);
		} else {
			mstruct.eval(eo);
		}
		if(mstruct.isPower() && mstruct[0].isInteger() && mstruct[1].isInteger() && mstruct[1].number().isPositive()&& !mstruct[0].number().isZero()) {
			Number nr;
			if(powmod(nr, mstruct[0].number(), mstruct[1].number(), vargs[1].number(), true)) {
				remove_overflow_message();
				mstruct = nr;
				return 1;
			}
		}

		if(mstruct.isMultiplication() && mstruct.size() > 0) {
			bool b = true;
			MathStructure mbak(mstruct);
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(!mstruct[i].isInteger() && (!mstruct[i].isPower() || !mstruct[i][0].isInteger() || !mstruct[i][1].isInteger() || mstruct[i][0].number().isZero())) {b = false; break;}
			}
			if(b) {
				remove_overflow_message();
				for(size_t i = 0; i < mstruct.size(); i++) {
					mstruct[i].transform(this);
					mstruct[i].addChild(vargs[1]);
				}
				mstruct.transform(this);
				mstruct.addChild(vargs[1]);
				return 1;
			} else {
				mstruct = mbak;
			}
		}
		CALCULATOR->endTemporaryStopMessages(eo.approximation == APPROXIMATION_EXACT || mstruct.isNumber());
		if(eo.approximation != APPROXIMATION_EXACT && !mstruct.isNumber()) {
			mstruct = vargs[0];
			mstruct.eval(eo);
		}
		if(!mstruct.isNumber() || !mstruct.number().isReal()) return -1;
		Number nr(mstruct.number());
		FR_FUNCTION_2b(rem)
		FR_FUNCTION_2c1
	}
	if(!vargs[0].number().isReal()) return 0;
	FR_FUNCTION_2(rem)
}
ModFunction::ModFunction() : MathFunction("mod", 2) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_TEST(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
int ModFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(!vargs[1].isInteger()) {
		if(!vargs[0].isNumber()) {
			mstruct = vargs[0];
			mstruct.eval(eo);
			if(!mstruct.isNumber() || !mstruct.number().isReal()) return -1;
			Number nr(mstruct.number());
			FR_FUNCTION_2b(mod)
			FR_FUNCTION_2c1
		}
	} else if(!vargs[0].isNumber()) {
		if(vargs[0].isPower() && vargs[0][0].isInteger() && vargs[0][1].isInteger() && vargs[0][1].number().isPositive() && !vargs[0][0].number().isZero()) {
			Number nr;
			if(powmod(nr, vargs[0][0].number(), vargs[0][1].number(), vargs[1].number(), false)) {mstruct = nr; return 1;}
		}
		mstruct = vargs[0];
		CALCULATOR->beginTemporaryStopMessages();
		if(eo.approximation != APPROXIMATION_EXACT) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			mstruct.eval(eo2);
		} else {
			mstruct.eval(eo);
		}
		if(mstruct.isPower() && mstruct[0].isInteger() && mstruct[1].isInteger() && mstruct[1].number().isPositive() && !mstruct[0].number().isZero()) {
			Number nr;
			if(powmod(nr, mstruct[0].number(), mstruct[1].number(), vargs[1].number(), false)) {
				remove_overflow_message();
				mstruct = nr;
				return 1;
			}
		}

		if(mstruct.isMultiplication() && mstruct.size() > 0) {
			bool b = true;
			MathStructure mbak(mstruct);
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(!mstruct[i].isInteger() && (!mstruct[i].isPower() || !mstruct[i][0].isInteger() || !mstruct[i][1].isInteger() || mstruct[i][0].number().isZero())) {b = false; break;}
			}
			if(b) {
				remove_overflow_message();
				for(size_t i = 0; i < mstruct.size(); i++) {
					mstruct[i].transform(this);
					mstruct[i].addChild(vargs[1]);
				}
				mstruct.transform(this);
				mstruct.addChild(vargs[1]);
				return 1;
			} else {
				mstruct = mbak;
			}
		}
		CALCULATOR->endTemporaryStopMessages(eo.approximation == APPROXIMATION_EXACT || mstruct.isNumber());
		if(eo.approximation != APPROXIMATION_EXACT && !mstruct.isNumber()) {
			mstruct = vargs[0];
			mstruct.eval(eo);
		}
		if(!mstruct.isNumber() || !mstruct.number().isReal()) return -1;
		Number nr(mstruct.number());
		FR_FUNCTION_2b(mod)
		FR_FUNCTION_2c1
	}
	if(!vargs[0].number().isReal()) return 0;
	FR_FUNCTION_2(mod)
}

ParallelFunction::ParallelFunction() : MathFunction("parallel", 2, -1) {}
int ParallelFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(!mstruct[i].representsNonZero(true)) mstruct[i].eval(eo);
		if(mstruct[i].representsZero(true)) {
			mstruct = vargs;
			mstruct.eval(eo);
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if((i2 > i && !mstruct[i2].representsNonZero(true)) || (i2 < mstruct.size() - 1 && !mstruct[i2].isUnitCompatible(mstruct[i2 + 1]))) {
					return 0;
				}
			}
			mstruct.setToChild(i + 1);
			return 1;
		}
		mstruct[i].inverse();
	}
	if(mstruct.size() == 0) {
		mstruct.clear();
	} else {
		if(mstruct.size() == 1) mstruct.setToChild(1);
		else mstruct.setType(STRUCT_ADDITION);
		mstruct.inverse();
	}
	return 1;
}

BernoulliFunction::BernoulliFunction() : MathFunction("bernoulli", 1, 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
	setDefaultValue(2, "0");
}
int BernoulliFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs.size() > 1 && !vargs[1].isZero()) {
		MathStructure m2(vargs[1]);
		replace_f_interval(m2, eo);
		replace_intervals_f(m2);
		mstruct.clear();
		Number bin, k, nmk(vargs[0].number()), nrB;
		while(k <= vargs[0].number()) {
			if(nmk.isEven() || nmk.isOne()) {
				nrB.set(nmk);
				if(!bin.binomial(vargs[0].number(), k) || !nrB.bernoulli() || !nrB.multiply(bin)) return 0;
				if(eo.approximation == APPROXIMATION_EXACT && nrB.isApproximate()) return 0;
				mstruct.add(nrB, true);
				mstruct.last().multiply(m2);
				mstruct.last().last().raise(k);
				mstruct.childUpdated(mstruct.size());
			}
			nmk--;
			k++;
		}
		if(mstruct.isAddition()) mstruct.delChild(1, true);
		return 1;
	}
	FR_FUNCTION(bernoulli)
}

TotientFunction::TotientFunction() : MathFunction("totient", 1, 1) {
	setArgumentDefinition(1, new IntegerArgument());
}
int TotientFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {mstruct.clear(); return 1;}
	if(vargs[0].number() <= 2 && vargs[0].number() >= -2) {mstruct.set(1, 1, 0); return 1;}
	mpz_t n, result, tmp, p_square, p;
	mpz_inits(n, result, tmp, p_square, p, NULL);
	mpz_set(n, mpq_numref(vargs[0].number().internalRational()));
	mpz_abs(n, n);
	mpz_set(result, n);
	size_t i = 0;
	while(true) {
		if(CALCULATOR->aborted()) {mpz_clears(n, result, tmp, p, p_square, NULL); return 0;}
		if(i < NR_OF_PRIMES) {
			if(i < NR_OF_SQUARE_PRIMES) {
				if(mpz_cmp_si(n, SQUARE_PRIMES[i]) < 0) break;
			} else {
				mpz_ui_pow_ui(p_square, PRIMES[i], 2);
				if(mpz_cmp(n, p_square) < 0) break;
			}
			if(mpz_divisible_ui_p(n, PRIMES[i])) {
				mpz_divexact_ui(n, n, PRIMES[i]);
				while(mpz_divisible_ui_p(n, PRIMES[i])) mpz_divexact_ui(n, n, PRIMES[i]);
				mpz_divexact_ui(tmp, result, PRIMES[i]);
				mpz_sub(result, result, tmp);
			}
			i++;
		} else {
			if(i == NR_OF_PRIMES) {mpz_set_si(p, PRIMES[i - 1]); i++;}
			mpz_add_ui(p, p, 2);
			mpz_pow_ui(p_square, p, 2);
			if(mpz_cmp(n, p_square) < 0) break;
			if(mpz_divisible_p(n, p)) {
				mpz_divexact(n, n, p);
				while(mpz_divisible_p(n, p)) mpz_divexact(n, n, p);
				mpz_divexact(tmp, result, p);
				mpz_sub(result, result, tmp);
			}
		}
	}
	if(mpz_cmp_ui(n, 1) > 0) {
		mpz_divexact(tmp, result, n);
		mpz_sub(result, result, tmp);
	}
	mstruct.clear();
	mstruct.number().setInternal(result);
	mpz_clears(n, result, tmp, p, p_square, NULL);
	return 1;
}

PolynomialUnitFunction::PolynomialUnitFunction() : MathFunction("punit", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialUnitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.set(vargs[0].polynomialUnit(vargs[1]), 0);
	return 1;
}
PolynomialPrimpartFunction::PolynomialPrimpartFunction() : MathFunction("primpart", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialPrimpartFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialPrimpart(vargs[1], mstruct, eo);
	return 1;
}
PolynomialContentFunction::PolynomialContentFunction() : MathFunction("pcontent", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialContentFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialContent(vargs[1], mstruct, eo);
	return 1;
}
CoeffFunction::CoeffFunction() : MathFunction("coeff", 2, 3) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "undefined");
}
int CoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].coefficient(vargs[2], vargs[1].number(), mstruct);
	return 1;
}
LCoeffFunction::LCoeffFunction() : MathFunction("lcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int LCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].lcoefficient(vargs[1], mstruct);
	return 1;
}
TCoeffFunction::TCoeffFunction() : MathFunction("tcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int TCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].tcoefficient(vargs[1], mstruct);
	return 1;
}
DegreeFunction::DegreeFunction() : MathFunction("degree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int DegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].degree(vargs[1]);
	return 1;
}
LDegreeFunction::LDegreeFunction() : MathFunction("ldegree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int LDegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].ldegree(vargs[1]);
	return 1;
}


BinFunction::BinFunction() : MathFunction("bin", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int BinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_BINARY;
	po.twos_complement = vargs[1].number().getBoolean();
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
OctFunction::OctFunction() : MathFunction("oct", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int OctFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_OCTAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
DecFunction::DecFunction() : MathFunction("dec", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int DecFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_DECIMAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
HexFunction::HexFunction() : MathFunction("hex", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int HexFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_HEXADECIMAL;
	po.hexadecimal_twos_complement = vargs[1].number().getBoolean();
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}

BaseFunction::BaseFunction() : MathFunction("base", 2, 3) {
	setArgumentDefinition(1, new TextArgument());
	Argument *arg = new Argument();
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
	IntegerArgument *arg2 = new IntegerArgument();
	arg2->setMin(&nr_zero);
	arg2->setMax(&nr_three);
	setArgumentDefinition(3, arg2);
	setArgumentDefinition(3, new TextArgument());
	setDefaultValue(3, "0");
}
int BaseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].isVector()) return 0;
	Number nbase;
	int idigits = 0;
	string sdigits;
	if(vargs.size() > 2) sdigits = vargs[2].symbol();
	if(sdigits.empty() || sdigits == "0" || sdigits == "auto") idigits = 0;
	else if(sdigits == "1") idigits = 1;
	else if(sdigits == "2") idigits = 2;
	else if(sdigits == "3" || sdigits == "Unicode" || sdigits == "unicode" || sdigits == "escaped") idigits = 3;
	else if(sdigits == "4" || sdigits == _("phoneword")) idigits = 4;
	else {
		size_t i = sdigits.find("|");
		if(i != string::npos && sdigits.find("|", i + 1) != string::npos) {
			idigits = -4;
		} else {
			i = sdigits.find(";");
			if(i != string::npos && sdigits.find(";", i + 1) != string::npos) {
				idigits = -3;
			} else {
				i = sdigits.find(",");
				if(i != string::npos && sdigits.find(",", i + 1) != string::npos) idigits = -2;
				else idigits = -1;
			}
		}
		i = sdigits.find(" ");
		if(i != string::npos && sdigits.find(" ", i + 1) != string::npos) remove_blanks(sdigits);
		if(idigits == -2 || idigits == -3) {
			if((sdigits[0] == LEFT_VECTOR_WRAP_CH && sdigits[sdigits.size() - 1] == RIGHT_VECTOR_WRAP_CH) || (sdigits[0] == LEFT_PARENTHESIS_CH && sdigits[sdigits.size() - 1] == RIGHT_PARENTHESIS_CH)) {
				sdigits = sdigits.substr(1, sdigits.size() - 2);
			}
		}
	}
	if(vargs[1].isNumber() && idigits == 0) {
		nbase = vargs[1].number();
	} else {
		mstruct = vargs[1];
		mstruct.eval(eo);
		if(mstruct.isVector()) return -2;
		if(idigits == 0 && !mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mstruct2(mstruct);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			mstruct2.eval(eo2);
			if(mstruct2.isNumber() && (mstruct2.number() > 62 || mstruct2.number() < -62)) {
				idigits = 3;
				CALCULATOR->endTemporaryStopMessages();
			} else if(mstruct2.isVector()) {
				mstruct = mstruct2;
				CALCULATOR->endTemporaryStopMessages(true);
				if(mstruct.isVector()) return -2;
			} else {
				CALCULATOR->endTemporaryStopMessages();
			}
		}
		if(mstruct.isNumber() && idigits == 0) {
			nbase = mstruct.number();
		} else {
			string number = vargs[0].symbol();
			size_t i_dot = number.length();
			vector<Number> digits;
			bool b_minus = false;
			if(idigits < 0) {
				vector<unordered_map<string, long int>> vdigits;
				string schar;
				long int v = 0;
				size_t d_i = 0;
				for(size_t i = 0; i < sdigits.length();) {
					if((idigits == -2 && sdigits[i] == ',') || (idigits == -3 && sdigits[i] == ';') || (idigits == -4 && sdigits[i] == '|')) {
						d_i = 0; v++; i++;
					} else {
						size_t l = 1;
						while(i + l < sdigits.length() && (signed char) sdigits[i + l] <= 0 && (unsigned char) sdigits[i + l] < 0xC0) l++;
						if(d_i == vdigits.size()) vdigits.resize(d_i + 1);
						vdigits[d_i][sdigits.substr(i, l)] = v;
						i += l;
						if(idigits < -1) d_i++;
						else v++;
					}
				}
				i_dot = number.length();
				for(size_t i = 0; i < number.length();) {
					size_t l = 1;
					while(i + l < number.length() && (signed char) number[i + l] <= 0 && (unsigned char) number[i + l] < 0xC0) l++;
					for(d_i = 0; d_i < vdigits.size(); d_i++) {
						unordered_map<string, long int>::iterator it = vdigits[d_i].find(number.substr(i, l));
						if(it != vdigits[d_i].end()) {
							digits.push_back(it->second);
							break;
						}
					}
					if(d_i == vdigits.size()) {
						if(l == 1 && (number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.'))) {
							if(i_dot == number.length()) i_dot = digits.size();
						} else if(!is_in(SPACES, number[i])) {
							CALCULATOR->error(true, _("Character \'%s\' was ignored in the number \"%s\" with base %s."), number.substr(i, l).c_str(), number.c_str(), format_and_print(mstruct).c_str(), NULL);
						}
					}
					i += l;
				}
			} else if(idigits == 4 || idigits == 5) {
				for(size_t i = 0; i < number.length();) {
					size_t l = 1;
					while(i + l < number.length() && (signed char) number[i + l] <= 0 && (unsigned char) number[i + l] < 0xC0) l++;
					char c = 0;
					if(l == 1) {
						c = number[i];
					} else if(l == 2 && (signed char) number[i] == -61) {
						if((signed char) number[i + 1] >= -128 && (signed char) number[i + 1] <= -122) c = 'A';
						else if((signed char) number[i + 1] == -121) c = 'C';
						else if((signed char) number[i + 1] >= -120 && (signed char) number[i + 1] <= -117) c = 'E';
						else if((signed char) number[i + 1] >= -116 && (signed char) number[i + 1] <= -113) c = 'I';
						//else if((signed char) number[i + 1] == -112) c = 'D';
						else if((signed char) number[i + 1] == -111) c = 'N';
						else if((signed char) number[i + 1] >= -110 && (signed char) number[i + 1] <= -106) c = 'O';
						else if((signed char) number[i + 1] == -104) c = 'O';
						else if((signed char) number[i + 1] >= -103 && (signed char) number[i + 1] <= -100) c = 'U';
						else if((signed char) number[i + 1] == -99) c = 'Y';
						//else if((signed char) number[i + 1] == -98) c = 'T';
						else if((signed char) number[i + 1] == -97) c = 'S';
						else if((signed char) number[i + 1] >= -96 && (signed char) number[i + 1] <= -90) c = 'a';
						else if((signed char) number[i + 1] == -89) c = 'c';
						else if((signed char) number[i + 1] >= -88 && (signed char) number[i + 1] <= -85) c = 'e';
						else if((signed char) number[i + 1] >= -84 && (signed char) number[i + 1] <= -81) c = 'i';
						//else if((signed char) number[i + 1] == -80) c = 'd';
						else if((signed char) number[i + 1] == -79) c = 'n';
						else if((signed char) number[i + 1] >= -78 && (signed char) number[i + 1] <= -74) c = 'o';
						else if((signed char) number[i + 1] == -72) c = 'o';
						else if((signed char) number[i + 1] >= -71 && (signed char) number[i + 1] <= -68) c = 'u';
						else if((signed char) number[i + 1] == -67) c = 'y';
						//else if((signed char) number[i + 1] == -66) c = 't';
						else if((signed char) number[i + 1] == -65) c = 'y';
					}
					if(c != 0) {
						if(c == '0') digits.push_back(0);
						else if(c == '1') digits.push_back(1);
						else if(is_in("2abcABC", c)) digits.push_back(2);
						else if(is_in("3defDEF", c)) digits.push_back(3);
						else if(is_in("4ghiGHI", c)) digits.push_back(4);
						else if(is_in("5jklJKL", c)) digits.push_back(5);
						else if(is_in("6mnoMNO", c)) digits.push_back(6);
						else if(is_in("7pqrsPQRS", c)) digits.push_back(7);
						else if(is_in("8tuvTUV", c)) digits.push_back(8);
						else if(is_in("9wxyzWXYZ", c)) digits.push_back(9);
					}
					i += l;
				}
			} else if(idigits <= 2) {
				remove_blanks(number);
				bool b_case = (idigits == 2);
				i_dot = number.length();
				for(size_t i = 0; i < number.length(); i++) {
					long int c = -1;
					if(number[i] >= '0' && number[i] <= '9') {
						c = number[i] - '0';
					} else if(number[i] >= 'a' && number[i] <= 'z') {
						if(b_case) c = number[i] - 'a' + 36;
						else c = number[i] - 'a' + 10;
					} else if(number[i] >= 'A' && number[i] <= 'Z') {
						c = number[i] - 'A' + 10;
					} else if(number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.')) {
						if(i_dot == number.length()) i_dot = digits.size();
					} else if(number[i] == '-' && digits.empty()) {
						b_minus = !b_minus;
					} else {
						string str_char = number.substr(i, 1);
						while(i + 1 < number.length() && (signed char) number[i + 1] < 0 && number[i + 1] && (unsigned char) number[i + 1] < 0xC0) {
							i++;
							str_char += number[i];
						}
						CALCULATOR->error(true, _("Character \'%s\' was ignored in the number \"%s\" with base %s."), str_char.c_str(), number.c_str(), format_and_print(mstruct).c_str(), NULL);
					}
					if(c >= 0) {
						digits.push_back(c);
					}
				}
			} else {
				for(size_t i = 0; i < number.length(); i++) {
					long int c = (unsigned char) number[i];
					bool b_esc = false;
					if(number[i] == '\\' && i < number.length() - 1) {
						i++;
						Number nrd;
						if(is_in(NUMBERS, number[i])) {
							size_t i2 = number.find_first_not_of(NUMBERS, i);
							if(i2 == string::npos) i2 = number.length();
							nrd.set(number.substr(i, i2 - i));
							i = i2 - 1;
							b_esc = true;
						} else if(number[i] == 'x' && i < number.length() - 1 && is_in(NUMBERS "ABCDEFabcdef", number[i + 1])) {
							i++;
							size_t i2 = number.find_first_not_of(NUMBERS "ABCDEFabcdef", i);
							if(i2 == string::npos) i2 = number.length();
							ParseOptions po;
							po.base = BASE_HEXADECIMAL;
							nrd.set(number.substr(i, i2 - i), po);
							i = i2 - 1;
							b_esc = true;
						}
						if(digits.empty() && (signed char) number[i] == (char) -30 && i + 3 < number.length() && number[i + 1] == (char) -120 && number[i + 2] == (char) -110) {
							i += 2;
							b_minus = !b_minus;
							b_esc = true;
						} else if(digits.empty() && number[i] == '-') {
							b_minus = !b_minus;
							b_esc = true;
						} else if(i_dot == number.size() && (number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.'))) {
							i_dot = digits.size();
							b_esc = true;
						} else if(b_esc) {
							digits.push_back(nrd);

						} else if(number[i] != '\\') {
							i--;
						}
					}
					if(!b_esc) {
						if((c & 0x80) != 0) {
							if(c<0xe0) {
								i++;
								if(i >= number.length()) return -2;
								c = ((c & 0x1f) << 6) | (((unsigned char) number[i]) & 0x3f);
							} else if(c<0xf0) {
								i++;
								if(i + 1 >= number.length()) return -2;
								c = (((c & 0xf) << 12) | ((((unsigned char) number[i]) & 0x3f) << 6)|(((unsigned char) number[i + 1]) & 0x3f));
								i++;
							} else {
								i++;
								if(i + 2 >= number.length()) return -2;
								c = ((c & 7) << 18) | ((((unsigned char) number[i]) & 0x3f) << 12) | ((((unsigned char) number[i + 1]) & 0x3f) << 6) | (((unsigned char) number[i + 2]) & 0x3f);
								i += 2;
							}
						}
						digits.push_back(c);
					}
				}
			}
			MathStructure mbase = mstruct;
			mstruct.clear();
			if(i_dot > digits.size()) i_dot = digits.size();
			for(size_t i = 0; i < digits.size(); i++) {
				long int exp = i_dot - 1 - i;
				MathStructure m;
				if(exp != 0) {
					m = mbase;
					m.raise(Number(exp, 1));
					m.multiply(digits[i]);
				} else {
					m.set(digits[i]);
				}
				if(mstruct.isZero()) mstruct = m;
				else mstruct.add(m, true);
			}
			if(b_minus) mstruct.negate();
			return 1;
		}
	}
	ParseOptions po = eo.parse_options;
	if(nbase.isInteger() && nbase >= 2 && nbase <= 36) {
		po.base = nbase.intValue();
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	} else {
		po.base = BASE_CUSTOM;
		Number cb_save = CALCULATOR->customInputBase();
		CALCULATOR->setCustomInputBase(nbase);
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
		CALCULATOR->setCustomInputBase(cb_save);
	}
	return 1;
}
RomanFunction::RomanFunction() : MathFunction("roman", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int RomanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].symbol().find_first_not_of("0123456789.:" SIGNS) == string::npos && vargs[0].symbol().find_first_not_of("0" SIGNS) != string::npos) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		PrintOptions po; po.base = BASE_ROMAN_NUMERALS;
		mstruct.eval(eo);
		mstruct.set(mstruct.print(po), true, true);
		return 1;
	}
	ParseOptions po = eo.parse_options;
	po.base = BASE_ROMAN_NUMERALS;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
BijectiveFunction::BijectiveFunction() : MathFunction("bijective", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int BijectiveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].symbol().find_first_not_of("0123456789.:" SIGNS) == string::npos && vargs[0].symbol().find_first_not_of(SIGNS) != string::npos) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		PrintOptions po; po.base = BASE_BIJECTIVE_26;
		mstruct.eval(eo);
		mstruct.set(mstruct.print(po), true, true);
		return 1;
	}
	ParseOptions po = eo.parse_options;
	po.base = BASE_BIJECTIVE_26;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
BinaryDecimalFunction::BinaryDecimalFunction() : MathFunction("bcd", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "1");
}
int BinaryDecimalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	bool b_packed = vargs[1].number().getBoolean();
	if(vargs[0].symbol().find_first_of("23456789") != string::npos) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		PrintOptions po; po.base = BASE_BINARY_DECIMAL; po.base_display = BASE_DISPLAY_NORMAL;
		mstruct.eval(eo);
		string str = mstruct.print(po);
		if(!b_packed) {
			size_t i = 0;
			while(i < str.length()) {
				str.insert(i, "0000");
				i += 9;
			}
		}
		mstruct.set(str, true, true);
		return 1;
	}
	ParseOptions po = eo.parse_options;
	po.base = BASE_BINARY_DECIMAL;
	string str = vargs[0].symbol();
	if(!b_packed) {
		remove_blanks(str);
		for(size_t i = 0; i < str.length(); i++) {
			if(i % 8 > 3) str[str.length() - i - 1] = ' ';
		}
		remove_blanks(str);
	}
	CALCULATOR->parse(&mstruct, str, po);
	return 1;
}
ImFunction::ImFunction() : MathFunction("im", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ImFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().imaginaryPart();
		return 1;
	} else if(mstruct.representsReal(!eo.keep_zero_units)) {
		mstruct.clear(true);
		return 1;
	} else if(mstruct.isUnit_exp()) {
		mstruct *= m_zero;
		mstruct.swapChildren(1, 2);
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].representsNegative()) {
		mstruct[0].negate();
		Number num = mstruct[1].number().numerator();
		num.rem(4);
		if(num == 3 || num == -1) mstruct.negate();
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].isNumber() && !mstruct[0].number().hasRealPart() && mstruct[0].number().imaginaryPartIsNonZero()) {
		Number nbase(mstruct[0].number().imaginaryPart());
		bool b_neg = nbase.isNegative();
		if(b_neg) nbase.negate();
		mstruct[0].set(nbase, true);
		mstruct[0].divide(nr_two);
		if(!mstruct[1].number().numeratorIsOne()) {
			Number nexp(mstruct[1].number());
			mstruct[1].number() /= nexp.numerator();
			if(nexp.isNegative()) {
				mstruct.inverse();
				b_neg = !b_neg;
				mstruct *= nr_half;
			}
			nexp.trunc();
			mstruct *= nbase;
			mstruct.last().raise(nexp);
			nexp /= 2;
			nexp.trunc();
			if(nexp.isOdd()) b_neg = !b_neg;
		}
		if(b_neg) mstruct.negate();
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() > 0) {
		if(mstruct[0].isNumber()) {
			Number nr = mstruct[0].number();
			mstruct.delChild(1, true);
			if(nr.hasImaginaryPart()) {
				if(nr.hasRealPart()) {
					MathStructure *madd = new MathStructure(mstruct);
					mstruct.transformById(FUNCTION_ID_RE);
					madd->transform(this);
					madd->multiply(nr.realPart());
					mstruct.multiply(nr.imaginaryPart());
					mstruct.add_nocopy(madd);
					return 1;
				}
				mstruct.transformById(FUNCTION_ID_RE);
				mstruct.multiply(nr.imaginaryPart());
				return 1;
			}
			mstruct.transform(this);
			mstruct.multiply(nr.realPart());
			return 1;
		}
		MathStructure *mreal = NULL;
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].representsReal(true)) {
				if(!mreal) {
					mreal = new MathStructure(mstruct[i]);
				} else {
					mstruct[i].ref();
					if(!mreal->isMultiplication()) mreal->transform(STRUCT_MULTIPLICATION);
					mreal->addChild_nocopy(&mstruct[i]);
				}
				mstruct.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(mreal) {
			if(mstruct.size() == 0) mstruct.clear(true);
			else if(mstruct.size() == 1) mstruct.setToChild(1, true);
			mstruct.transform(this);
			mstruct.multiply_nocopy(mreal);
			return 1;
		}
	} else if(mstruct.isAddition() && mstruct.size() > 1) {
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].representsReal(true)) {
				mstruct.delChild(i + 1);
			} else {
				mstruct[i].transform(this);
				i++;
			}
		}
		if(mstruct.size() == 0) mstruct.clear(true);
		else if(mstruct.size() == 1) mstruct.setToChild(1, true);
		return 1;
	}
	if(represents_imaginary(mstruct)) {
		mstruct *= nr_minus_i;
		return 1;
	}
	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct.transform(this);
		mstruct.negate();
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
bool ImFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool ImFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ImFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ImFunction::representsUndefined(const MathStructure&) const {return false;}

ReFunction::ReFunction() : MathFunction("re", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ReFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().realPart();
		return 1;
	} else if(mstruct.representsReal(true)) {
		return 1;
	} else if(represents_imaginary(mstruct, !eo.keep_zero_units)) {
		mstruct.clear(true);
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].representsNegative()) {
		mstruct.clear(true);
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].isNumber() && !mstruct[0].number().hasRealPart() && mstruct[0].number().imaginaryPartIsNonZero()) {
		Number nbase(mstruct[0].number().imaginaryPart());
		if(nbase.isNegative()) nbase.negate();
		mstruct[0].set(nbase, true);
		mstruct[0].divide(nr_two);
		if(!mstruct[1].number().numeratorIsOne()) {
			Number nexp(mstruct[1].number());
			mstruct[1].number() /= nexp.numerator();
			if(nexp.isNegative()) {
				mstruct.inverse();
				mstruct *= nr_half;
			}
			Number nexp2(nexp);
			nexp.trunc();
			mstruct *= nbase;
			mstruct.last().raise(nexp);
			nexp2 /= 2;
			nexp2.round();
			if(nexp2.isOdd()) mstruct.negate();
		}
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() > 0) {
		if(mstruct[0].isNumber()) {
			Number nr = mstruct[0].number();
			mstruct.delChild(1, true);
			if(nr.hasImaginaryPart()) {
				if(nr.hasRealPart()) {
					MathStructure *madd = new MathStructure(mstruct);
					mstruct.transformById(FUNCTION_ID_IM);
					madd->transform(this);
					madd->multiply(nr.realPart());
					mstruct.multiply(-nr.imaginaryPart());
					mstruct.add_nocopy(madd);
					return 1;
				}
				mstruct.transformById(FUNCTION_ID_IM);
				mstruct.multiply(-nr.imaginaryPart());
				return 1;
			}
			mstruct.transform(this);
			mstruct.multiply(nr.realPart());
			return 1;
		}
		MathStructure *mreal = NULL;
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].representsReal(true)) {
				if(!mreal) {
					mreal = new MathStructure(mstruct[i]);
				} else {
					mstruct[i].ref();
					if(!mreal->isMultiplication()) mreal->transform(STRUCT_MULTIPLICATION);
					mreal->addChild_nocopy(&mstruct[i]);
				}
				mstruct.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(mreal) {
			if(mstruct.size() == 0) mstruct.clear(true);
			else if(mstruct.size() == 1) mstruct.setToChild(1, true);
			mstruct.transform(this);
			mstruct.multiply_nocopy(mreal);
			return 1;
		}
	} else if(mstruct.isAddition() && mstruct.size() > 1) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size();) {
			if(represents_imaginary(mstruct[i])) {
				mstruct.delChild(i + 1);
				b = true;
			} else {
				if(!mstruct[i].representsReal()) {
					mstruct[i].transform(this);
					b = true;
				}
				i++;
			}
		}
		if(b) {
			if(mstruct.size() == 0) mstruct.clear(true);
			else if(mstruct.size() == 1) mstruct.setToChild(1, true);
			return 1;
		}
	}
	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct.transform(this);
		mstruct.negate();
		return 1;
	}
	return -1;
}
bool ReFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsPositive(allow_units);}
bool ReFunction::representsNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNegative(allow_units);}
bool ReFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool ReFunction::representsNonPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonPositive(allow_units);}
bool ReFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsInteger(allow_units);}
bool ReFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool ReFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsRational(allow_units);}
bool ReFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool ReFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool ReFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ReFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsReal(allow_units) && vargs[0].representsNonZero(true);}
bool ReFunction::representsEven(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsEven(allow_units);}
bool ReFunction::representsOdd(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsOdd(allow_units);}
bool ReFunction::representsUndefined(const MathStructure&) const {return false;}

bool represents_imre(const MathStructure &m) {
	switch(m.type()) {
		case STRUCT_NUMBER: {return m.number().imaginaryPartIsNonZero() && m.number().realPartIsNonZero();}
		case STRUCT_VARIABLE: {return m.variable()->isKnown() && represents_imre(((KnownVariable*) m.variable())->get());}
		case STRUCT_POWER: {
			return m[1].isNumber() && m[1].number().isRational() && !m[1].number().isInteger() && (m[0].representsComplex() || (!m[1].number().denominatorIsTwo() && m[0].representsNegative()));
		}
		default: {return false;}
	}
	return false;
}

ArgFunction::ArgFunction() : MathFunction("arg", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool ArgFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool ArgFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool ArgFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
int ArgFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;

	MathStructure msave;

	arg_test_non_number:
	if(!mstruct.isNumber()) {
		if(mstruct.representsPositive(true)) {
			mstruct.clear();
			if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
			return 1;
		}
		if(mstruct.representsNegative(true)) {
			set_fraction_of_turn(mstruct, eo, 1, 2);
			return 1;
		}
		if(!msave.isZero()) {
			mstruct = msave;
			return -1;
		}
		if(mstruct.isMultiplication()) {
			bool b = false;
			for(size_t i = 0; i < mstruct.size();) {
				if(mstruct[i].representsPositive()) {
					mstruct.delChild(i + 1);
					b = true;
				} else {
					if(!mstruct[i].isMinusOne() && mstruct[i].representsNegative()) {
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
		if(mstruct.isMultiplication()) {
			bool b = false;
			if(mstruct.size() == 2) {
				if(represents_imre(mstruct[0])) {
					b = mstruct[1].representsComplex();
				} else if(represents_imre(mstruct[1])) {
					b = mstruct[0].representsComplex();
				}
			} else {
				b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					if(!represents_imre(mstruct[i])) {
						b = false;
					}
				}
			}
			if(b) {
				for(size_t i = 0; i < mstruct.size(); i++) {
					mstruct[i].transform(this);
				}
				mstruct.setType(STRUCT_ADDITION);
				return 1;
			}
		}
		if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable()->id() == VARIABLE_ID_E && mstruct[1].isNumber() && mstruct[1].number().hasImaginaryPart() && !mstruct[1].number().hasRealPart()) {
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				Number nr(*mstruct[1].number().internalImaginary());
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
					mstruct = mstruct[1].number().imaginaryPart();
					if(!nr.isZero()) {
						mstruct += nr;
						mstruct.last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					}
					return true;
				}
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
		}
		if(mstruct.isPower() && mstruct[0].isMinusOne() && mstruct[1].representsFraction() && mstruct[1].representsPositive()) {
			mstruct[0] = CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct.setType(STRUCT_MULTIPLICATION);
			return true;
		} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne() && mstruct[1].isPower() && mstruct[1][0].isMinusOne() && mstruct[1][1].representsFraction() && mstruct[1][1].representsPositive()) {
			mstruct[1][0] = CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct[1][1].negate();
			mstruct[1][1] += m_one;
			mstruct[1].setType(STRUCT_MULTIPLICATION);
			return true;
		}
		MathStructure m_im(CALCULATOR->getFunctionById(FUNCTION_ID_IM), &mstruct, NULL);
		CALCULATOR->beginTemporaryStopMessages();
		m_im.eval(eo);
		if(!m_im.containsFunctionId(FUNCTION_ID_IM)) {
			MathStructure m_re(CALCULATOR->getFunctionById(FUNCTION_ID_RE), &mstruct, NULL);
			m_re.eval(eo);
			if(!m_re.containsFunctionId(FUNCTION_ID_RE)) {
				ComparisonResult cr_im = m_im.compare(m_zero);
				ComparisonResult cr_re = m_re.compare(m_zero);
				if(cr_im == COMPARISON_RESULT_EQUAL) {
					if(cr_re == COMPARISON_RESULT_LESS) {
						mstruct.clear();
						if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
						CALCULATOR->endTemporaryStopMessages(true);
						return 1;
					} else if(cr_re == COMPARISON_RESULT_GREATER) {
						set_fraction_of_turn(mstruct, eo, 1, 2);
						CALCULATOR->endTemporaryStopMessages(true);
						return 1;
					}
				} else if(COMPARISON_IS_NOT_EQUAL(cr_im)) {
					if(cr_re == COMPARISON_RESULT_EQUAL) {
						int i_sgn = 0;
						if(cr_im == COMPARISON_RESULT_LESS) i_sgn = 1;
						else if(cr_im == COMPARISON_RESULT_GREATER) i_sgn = -1;
						if(i_sgn != 0) {
							set_fraction_of_turn(mstruct, eo, 1, 4);
							if(i_sgn < 0) mstruct.negate();
							CALCULATOR->endTemporaryStopMessages(true);
							return 1;
						}
					} else if(cr_re == COMPARISON_RESULT_GREATER) {
						if(cr_im == COMPARISON_RESULT_GREATER) {
							m_im.divide(m_re);
							mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &m_im, NULL);
							add_fraction_of_turn(mstruct, eo, -1, 2);
							CALCULATOR->endTemporaryStopMessages(true);
							return 1;
						} else if(cr_im == COMPARISON_RESULT_LESS) {
							m_im.divide(m_re);
							mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &m_im, NULL);
							add_fraction_of_turn(mstruct, eo, 1, 2);
							CALCULATOR->endTemporaryStopMessages(true);
							return 1;
						}
					} else if(cr_re == COMPARISON_RESULT_LESS) {
						m_im.divide(m_re);
						mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &m_im, NULL);
						CALCULATOR->endTemporaryStopMessages(true);
						return 1;
					}
				}
			}
		}
		CALCULATOR->endTemporaryStopMessages();
		if(eo.approximation == APPROXIMATION_EXACT) {
			msave = mstruct;
			if(!test_eval(mstruct, eo)) {
				mstruct = msave;
				return -1;
			}
		}
	}
	if(mstruct.isNumber()) {
		if(!mstruct.number().hasImaginaryPart()) {
			if(!mstruct.number().isNonZero()) {
				if(!msave.isZero()) mstruct = msave;
				return -1;
			}
			if(mstruct.number().isNegative()) {
				set_fraction_of_turn(mstruct, eo, 1, 2);
			} else {
				mstruct.clear();
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
			}
		} else if(!mstruct.number().hasRealPart() && mstruct.number().imaginaryPartIsNonZero()) {
			bool b_neg = mstruct.number().imaginaryPartIsNegative();
			set_fraction_of_turn(mstruct, eo, 1, 4);
			if(b_neg) mstruct.negate();
		} else if(!msave.isZero()) {
			mstruct = msave;
			return -1;
		} else if(!mstruct.number().realPartIsNonZero() || (!mstruct.number().imaginaryPartIsNonZero() && mstruct.number().realPartIsNegative())) {
			Number nr(mstruct.number());
			if(!nr.arg() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
				return -1;
			} else {
				mstruct.set(nr);
				multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
				return 1;
			}
		} else {
			MathStructure new_nr(mstruct.number().imaginaryPart());
			if(!new_nr.number().divide(mstruct.number().realPart())) return -1;
			if(mstruct.number().realPartIsNegative()) {
				if(mstruct.number().imaginaryPartIsNegative()) {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					add_fraction_of_turn(mstruct, eo, -1, 2);
				} else {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					add_fraction_of_turn(mstruct, eo, 1, 2);
				}
			} else {
				mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
			}
		}
		return 1;
	}
	if(!msave.isZero()) {
		goto arg_test_non_number;
	}
	return -1;
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

#define IS_NUMBER_FUNCTION(x, y) x::x() : MathFunction(#y, 1) {\
	Argument *arg = new Argument();\
	arg->setHandleVector(true);\
	setArgumentDefinition(1, arg);\
}\
int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {\
	mstruct = vargs[0];\
	if(!mstruct.isNumber()) mstruct.eval(eo);\
	if(mstruct.isVector()) return -1;\
	if(mstruct.isNumber() && mstruct.number().y()) {mstruct.number().setTrue();} else {mstruct.clear(); mstruct.number().setFalse();}\
	return 1;\
}

IS_NUMBER_FUNCTION(IsIntegerFunction, isInteger)
IS_NUMBER_FUNCTION(IsRealFunction, isReal)
IS_NUMBER_FUNCTION(IsRationalFunction, isRational)

IEEE754FloatFunction::IEEE754FloatFunction() : MathFunction("float", 1, 4) {
	Argument *arg = new TextArgument();
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG);
	Number nmin(8, 1);
	iarg->setMin(&nmin);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "32");
	setArgumentDefinition(3,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
	setArgumentDefinition(4,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(4, "0");
	setCondition("\\z<\\y-1 && \\a<\\y");
}
int IEEE754FloatFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	string sbin = vargs[0].symbol();
	unsigned int bits = vargs[1].number().uintValue();
	unsigned int expbits = vargs[2].number().uintValue();
	unsigned int sgnpos = vargs[3].number().uintValue();
	remove_blanks(sbin);
	if(sbin.find_first_not_of("01") != string::npos) {
		MathStructure m;
		CALCULATOR->parse(&m, vargs[0].symbol(), eo.parse_options);
		m.eval(eo);
		if(!m.isInteger() || !m.number().isNonNegative()) return 0;
		PrintOptions po;
		po.base = BASE_BINARY;
		po.twos_complement = false;
		po.binary_bits = bits;
		po.min_exp = 0;
		po.base_display = BASE_DISPLAY_NONE;
		sbin = m.print(po);
		remove_blanks(sbin);
	}
	Number nr;
	int ret = from_float(nr, sbin, bits, expbits, sgnpos);
	if(ret == 0) return 0;
	if(ret < 0) mstruct.setUndefined();
	else mstruct = nr;
	return 1;
}
IEEE754FloatBitsFunction::IEEE754FloatBitsFunction() : MathFunction("floatBits", 1, 4) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG);
	Number nmin(8, 1);
	iarg->setMin(&nmin);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "32");
	setArgumentDefinition(3,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
	setArgumentDefinition(4,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(4, "0");
	setCondition("\\z<\\y-1 && \\a<\\y");
}
int IEEE754FloatBitsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	unsigned int bits = vargs[1].number().uintValue();
	unsigned int expbits = vargs[2].number().uintValue();
	unsigned int sgnpos = vargs[3].number().uintValue();
	string sbin = to_float(vargs[0].number(), bits, expbits, sgnpos);
	if(sbin.empty()) return 0;
	ParseOptions pa;
	pa.base = BASE_BINARY;
	Number nr(sbin, pa);
	if(nr.isInfinite() && !vargs[0].number().isInfinite()) CALCULATOR->error(false, _("Floating point overflow"), NULL);
	else if(nr.isZero() && !vargs[0].isZero()) CALCULATOR->error(false, _("Floating point underflow"), NULL);
	mstruct = nr;
	return 1;
}
IEEE754FloatComponentsFunction::IEEE754FloatComponentsFunction() : MathFunction("floatParts", 1, 4) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG);
	Number nmin(8, 1);
	iarg->setMin(&nmin);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "32");
	setArgumentDefinition(3,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
	setArgumentDefinition(4,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(4, "0");
	setCondition("\\z<\\y-1 && \\a<\\y");
}
int IEEE754FloatComponentsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	unsigned int bits = vargs[1].number().uintValue();
	unsigned int expbits = vargs[2].number().uintValue();
	unsigned int sgnpos = vargs[3].number().uintValue();
	if(expbits == 0) expbits = standard_expbits(bits);
	string sbin = to_float(vargs[0].number(), bits, expbits, sgnpos);
	if(sbin.empty()) return 0;
	Number sign, exponent, significand;
	if(sbin[0] == '0') sign = 1;
	else sign = -1;
	ParseOptions pa;
	pa.base = BASE_BINARY;
	exponent.set(sbin.substr(1, expbits), pa);
	Number expbias(2);
	expbias ^= (expbits - 1);
	expbias--;
	bool subnormal = exponent.isZero();
	exponent -= expbias;
	if(exponent > expbias) return 0;
	if(subnormal) exponent++;
	if(subnormal) significand.set(string("0.") + sbin.substr(1 + expbits), pa);
	else significand.set(string("1.") + sbin.substr(1 + expbits), pa);
	if(subnormal && significand.isZero()) exponent.clear();
	mstruct.clearVector();
	mstruct.addChild(sign);
	mstruct.addChild(exponent);
	mstruct.addChild(significand);
	return 1;
}
IEEE754FloatValueFunction::IEEE754FloatValueFunction() : MathFunction("floatValue", 1, 4) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG);
	Number nmin(8, 1);
	iarg->setMin(&nmin);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "32");
	setArgumentDefinition(3,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
	setArgumentDefinition(4,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(4, "0");
	setCondition("\\z<\\y-1 && \\a<\\y");
}
int IEEE754FloatValueFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	unsigned int bits = vargs[1].number().uintValue();
	unsigned int expbits = vargs[2].number().uintValue();
	unsigned int sgnpos = vargs[3].number().uintValue();
	string sbin = to_float(vargs[0].number(), bits, expbits, sgnpos);
	if(sbin.empty()) return 0;
	Number nr;
	int ret = from_float(nr, sbin, bits, expbits, sgnpos);
	if(ret == 0) mstruct.setUndefined();
	else mstruct = nr;
	return 1;
}
IEEE754FloatErrorFunction::IEEE754FloatErrorFunction() : MathFunction("floatError", 1, 4) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG);
	Number nmin(8, 1);
	iarg->setMin(&nmin);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "32");
	setArgumentDefinition(3,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
	setArgumentDefinition(4,  new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(4, "0");
	setCondition("\\z<\\y-1 && \\a<\\y");
}
int IEEE754FloatErrorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	unsigned int bits = vargs[1].number().uintValue();
	unsigned int expbits = vargs[2].number().uintValue();
	unsigned int sgnpos = vargs[3].number().uintValue();
	string sbin = to_float(vargs[0].number(), bits, expbits, sgnpos);
	if(sbin.empty()) return 0;
	Number nr;
	int ret = from_float(nr, sbin, bits, expbits, sgnpos);
	if(ret == 0) return 0;
	if(ret < 0 || (vargs[0].number().isInfinite() && nr.isInfinite())) {
		mstruct.clear();
	} else {
		nr -= vargs[0].number();
		nr.abs();
		mstruct = nr;
	}
	return 1;
}

UpperEndPointFunction::UpperEndPointFunction() : MathFunction("upperEndpoint", 1) {
	setArgumentDefinition(1, new NumberArgument());
}
int UpperEndPointFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].number().upperEndPoint();
	return 1;
}
LowerEndPointFunction::LowerEndPointFunction() : MathFunction("lowerEndpoint", 1) {
	setArgumentDefinition(1, new NumberArgument());
}
int LowerEndPointFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].number().lowerEndPoint();
	return 1;
}
MidPointFunction::MidPointFunction() : MathFunction("midpoint", 1) {
	setArgumentDefinition(1, new NumberArgument());
}
int MidPointFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	Number nr(vargs[0].number());
	nr.intervalToMidValue();
	mstruct = nr;
	return 1;
}
GetUncertaintyFunction::GetUncertaintyFunction() : MathFunction("errorPart", 1, 2) {
	setArgumentDefinition(1, new NumberArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int GetUncertaintyFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[1].number().getBoolean()) mstruct = vargs[0].number().relativeUncertainty();
	else mstruct = vargs[0].number().uncertainty();
	return 1;
}
