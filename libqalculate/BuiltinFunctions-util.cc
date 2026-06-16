/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016, 2018, 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
using std::ostream;
using std::endl;

#define REPRESENTS_FUNCTION(x, y) \
	x::x() : MathFunction(#y, 1) {}\
	int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {\
		if(vargs[0].y()) {\
			mstruct.clear();\
			mstruct.number().setTrue();\
			return 1;\
		}\
		mstruct = vargs[0];\
		mstruct.eval(eo);\
		if(mstruct.y()) {\
			mstruct.clear();\
			mstruct.number().setTrue();\
			return 1;\
		}\
		if(eo.approximation != APPROXIMATION_EXACT) {\
			mstruct = vargs[0];\
			EvaluationOptions eo2 = eo;\
			eo2.approximation = APPROXIMATION_EXACT;\
			mstruct.eval(eo2);\
			if(mstruct.y()) {\
				mstruct.clear();\
				mstruct.number().setTrue();\
				return 1;\
			}\
		}\
		mstruct.clear();\
		mstruct.number().setFalse();\
		return 1;\
	}

bool replace_infinity_v(MathStructure &m) {
	if(m.isVariable() && m.variable()->isKnown() && ((KnownVariable*) m.variable())->get().isNumber() && ((KnownVariable*) m.variable())->get().number().isInfinite(false)) {
		m = ((KnownVariable*) m.variable())->get();
		return true;
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_infinity_v(m[i])) b_ret = true;
	}
	return b_ret;
}
bool contains_infinity_v(const MathStructure &m) {
	if(m.isVariable() && m.variable()->isKnown() && ((KnownVariable*) m.variable())->get().isNumber() && ((KnownVariable*) m.variable())->get().number().isInfinite(false)) {
		return true;
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_infinity_v(m[i])) b_ret = true;
	}
	return b_ret;
}

bool create_interval(MathStructure &mstruct, const MathStructure &m1, const MathStructure &m2, bool exclude_limits) {
	if(contains_infinity_v(m1) || contains_infinity_v(m2)) {
		MathStructure m1b(m1), m2b(m2);
		if(replace_infinity_v(m1b) || replace_infinity_v(m2b)) return create_interval(mstruct, m1b, m2b, exclude_limits);
	}
	if(m1 == m2) {
		mstruct.set(m1, true);
		return 1;
	} else if(m1.isNumber() && m2.isNumber()) {
		Number nr;
		if(!nr.setInterval(m1.number(), m2.number())) return false;
		if(exclude_limits) {
			mpfr_nextbelow(nr.internalUpperFloat());
			mpfr_nextabove(nr.internalLowerFloat());
		}
		mstruct.set(nr, true);
		return true;
	} else if(m1.isMultiplication() && m2.isMultiplication() && m1.size() > 1 && m2.size() > 1) {
		size_t i0 = 0, i1 = 0;
		if(m1[0].isNumber()) i0++;
		if(m2[0].isNumber()) i1++;
		if(i0 == 0 && i1 == 0) return false;
		if(m1.size() - i0 != m2.size() - i1) return false;
		for(size_t i = 0; i < m1.size() - i0; i++) {
			if(!m1[i + i0].equals(m2[i + i1], true)) {
				return false;
			}
		}
		Number nr;
		if(!nr.setInterval(i0 == 1 ? m1[0].number() : nr_one, i1 == 1 ? m2[0].number() : nr_one)) return false;
		if(exclude_limits) {
			mpfr_nextbelow(nr.internalUpperFloat());
			mpfr_nextabove(nr.internalLowerFloat());
		}
		mstruct.set(m1, true);
		if(i0 == 1) mstruct.delChild(1, true);
		mstruct *= nr;
		mstruct.evalSort(false);
		return 1;
	} else if(m1.isAddition() && m2.isAddition() && m1.size() > 1 && m2.size() > 1) {
		size_t i0 = 0, i1 = 0;
		if(m1.last().isNumber()) i0++;
		if(m2.last().isNumber()) i1++;
		if(i0 == 0 && i1 == 0) return false;
		if(m1.size() - i0 != m2.size() - i1) return false;
		for(size_t i = 0; i < m1.size() - i0; i++) {
			if(!m1[i].equals(m2[i], true)) {
				return false;
			}
		}
		Number nr;
		if(!nr.setInterval(i0 == 1 ? m1.last().number() : nr_one, i1 == 1 ? m2.last().number() : nr_one)) return false;
		if(exclude_limits) {
			mpfr_nextbelow(nr.internalUpperFloat());
			mpfr_nextabove(nr.internalLowerFloat());
		}
		mstruct.set(m1, true);
		if(i0 == 1) mstruct.delChild(mstruct.size(), true);
		mstruct += nr;
		mstruct.evalSort(false);
		return true;
	} else if(m1.isMultiplication() && m1.size() == 2 && m1[0].isNumber() && m2.equals(m1[1], true)) {
		Number nr;
		if(!nr.setInterval(m1[0].number(), nr_one)) return false;
		if(exclude_limits) {
			mpfr_nextbelow(nr.internalUpperFloat());
			mpfr_nextabove(nr.internalLowerFloat());
		}
		mstruct.set(nr, true);
		mstruct *= m2;
		return true;
	} else if(m2.isMultiplication() && m2.size() == 2 && m2[0].isNumber() && m1.equals(m2[1], true)) {
		Number nr;
		if(!nr.setInterval(nr_one, m2[0].number())) return false;
		if(exclude_limits) {
			mpfr_nextbelow(nr.internalUpperFloat());
			mpfr_nextabove(nr.internalLowerFloat());
		}
		mstruct.set(nr, true);
		mstruct *= m1;
		return true;
	}
	return false;
}

IntervalFunction::IntervalFunction() : MathFunction("interval", 2, 3) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "0");
}
int IntervalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(create_interval(mstruct, vargs[0], vargs[1], vargs.size() >= 3 ? vargs[2].number().getBoolean() : false)) return 1;
	MathStructure marg1(vargs[0]);
	marg1.eval(eo);
	MathStructure marg2(vargs[1]);
	marg2.eval(eo);
	if(create_interval(mstruct, marg1, marg2, vargs.size() >= 3 ? vargs[2].number().getBoolean() : false)) return 1;
	return 0;
}
bool IntervalFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsPositive(allow_units) && vargs[0].representsPositive(allow_units);}
bool IntervalFunction::representsNegative(const MathStructure&vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsNegative(allow_units) && vargs[1].representsNegative(allow_units);}
bool IntervalFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsNonNegative(allow_units) && vargs[1].representsNonNegative(allow_units);}
bool IntervalFunction::representsNonPositive(const MathStructure&vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsNonPositive(allow_units) && vargs[1].representsNonPositive(allow_units);}
bool IntervalFunction::representsInteger(const MathStructure &, bool) const {return false;}
bool IntervalFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsNumber(allow_units) && vargs[1].representsNumber(allow_units);}
bool IntervalFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsRational(allow_units) && vargs[1].representsRational(allow_units);}
bool IntervalFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsReal(allow_units) && vargs[1].representsReal(allow_units);}
bool IntervalFunction::representsNonComplex(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[0].representsNonComplex(allow_units) && vargs[1].representsNonComplex(allow_units);}
bool IntervalFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool IntervalFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return representsPositive(vargs, allow_units) || representsNegative(vargs, allow_units);}
bool IntervalFunction::representsEven(const MathStructure&, bool) const {return false;}
bool IntervalFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool IntervalFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 2 && (vargs[0].representsUndefined() || vargs[1].representsUndefined());}


bool set_uncertainty(MathStructure &mstruct, MathStructure &munc, const EvaluationOptions &eo, bool do_eval) {
	if(munc.isFunction() && munc.function()->id() == FUNCTION_ID_ABS && munc.size() == 1) {
		munc.setToChild(1, true);
	}
	test_munc:
	if(munc.isNumber()) {
		if(munc.isZero()) {
			return true;
		} else if(mstruct.isNumber()) {
			mstruct.number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
			mstruct.numberUpdated();
			return true;
		} else if(mstruct.isAddition()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isNumber()) {
					mstruct[i].number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
					mstruct[i].numberUpdated();
					mstruct.childUpdated(i + 1);
					return true;
				}
			}
		}
		mstruct.add(m_zero, true);
		mstruct.last().number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
		mstruct.last().numberUpdated();
		mstruct.childUpdated(mstruct.size());
		return true;
	} else {
		if(munc.isMultiplication()) {
			if(!munc[0].isNumber()) {
				munc.insertChild(m_one, 1);
			}
		} else {
			munc.transform(STRUCT_MULTIPLICATION);
			munc.insertChild(m_one, 1);
		}
		if(munc.isMultiplication()) {
			if(munc.size() == 2) {
				if(mstruct.isMultiplication() && mstruct[0].isNumber() && (munc[1] == mstruct[1] || (munc[1].isFunction() && munc[1].function()->id() == FUNCTION_ID_ABS && munc[1].size() == 1 && mstruct[1] == munc[1][0]))) {
					mstruct[0].number().setUncertainty(munc[0].number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
					mstruct[0].numberUpdated();
					mstruct.childUpdated(1);
					return true;
				} else if(mstruct.equals(munc[1]) || (munc[1].isFunction() && munc[1].function()->id() == FUNCTION_ID_ABS && munc[1].size() == 1 && mstruct.equals(munc[1][0]))) {
					mstruct.transform(STRUCT_MULTIPLICATION);
					mstruct.insertChild(m_one, 1);
					mstruct[0].number().setUncertainty(munc[0].number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
					mstruct[0].numberUpdated();
					mstruct.childUpdated(1);
					return true;
				}
			} else if(mstruct.isMultiplication()) {
				size_t i2 = 0;
				if(mstruct[0].isNumber()) i2++;
				if(mstruct.size() + 1 - i2 == munc.size()) {
					bool b = true;
					for(size_t i = 1; i < munc.size(); i++, i2++) {
						if(!munc[i].equals(mstruct[i2]) && !(munc[i].isFunction() && munc[i].function()->id() == FUNCTION_ID_ABS && munc[i].size() == 1 && mstruct[i2] == munc[i][0])) {
							b = false;
							break;
						}
					}
					if(b) {
						if(!mstruct[0].isNumber()) {
							mstruct.insertChild(m_one, 1);
						}
						mstruct[0].number().setUncertainty(munc[0].number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
						mstruct[0].numberUpdated();
						mstruct.childUpdated(1);
						return true;
					}
				}
			}
			if(do_eval) {
				bool b = false;
				for(size_t i = 0; i < munc.size(); i++) {
					if(munc[i].isFunction() && munc[i].function()->id() == FUNCTION_ID_ABS && munc[i].size() == 1) {
						munc[i].setToChild(1);
						b = true;
					}
				}
				if(b) {
					munc.eval(eo);
					goto test_munc;
				}
			}
		}
	}
	return false;
}

UncertaintyFunction::UncertaintyFunction() : MathFunction("uncertainty", 2, 3) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "1");
}
int UncertaintyFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	MathStructure munc(vargs[1]);
	mstruct.eval(eo);
	munc.eval(eo);
	if(vargs[2].number().getBoolean()) {
		if(munc.isNumber() && mstruct.isNumber()) {
			mstruct.number().setRelativeUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
			mstruct.numberUpdated();
			return 1;
		}
		mstruct = vargs[0];
		mstruct *= m_one;
		mstruct.last() -= vargs[1];
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		MathStructure *m2 = new MathStructure(vargs[0]);
		m2->multiply(m_one);
		m2->last() += vargs[1];
		mstruct.addChild_nocopy(m2);
		mstruct.addChild(m_zero);
		return 1;
	} else {
		if(set_uncertainty(mstruct, munc, eo, true)) return 1;
		mstruct = vargs[0];
		mstruct -= vargs[1];
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		MathStructure *m2 = new MathStructure(vargs[0]);
		m2->add(vargs[1]);
		mstruct.addChild_nocopy(m2);
		mstruct.addChild(m_zero);
		return 1;
	}
	return 0;
}

AsciiFunction::AsciiFunction() : MathFunction("code", 1, 3) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new TextArgument());
	setDefaultValue(2, "UTF-32");
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "1");
}
int AsciiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].symbol().empty()) {
		return false;
	}
	const string &str = vargs[0].symbol();
	int i_encoding = -1;
	if(equalsIgnoreCase(vargs[1].symbol(), "UTF-32") || equalsIgnoreCase(vargs[1].symbol(), "UTF32") || equalsIgnoreCase(vargs[1].symbol(), "UTF−32") || vargs[1].symbol() == "2") i_encoding = 2;
	else if(equalsIgnoreCase(vargs[1].symbol(), "UTF-16") || equalsIgnoreCase(vargs[1].symbol(), "UTF16") || equalsIgnoreCase(vargs[1].symbol(), "UTF−16") || vargs[1].symbol() == "1") i_encoding = 1;
	else if(equalsIgnoreCase(vargs[1].symbol(), "UTF-8") || equalsIgnoreCase(vargs[1].symbol(), "UTF8") || equalsIgnoreCase(vargs[1].symbol(), "UTF−8") || equalsIgnoreCase(vargs[1].symbol(), "ascii") || vargs[1].symbol() == "0") i_encoding = 0;
	switch(i_encoding) {
		case 0: {
			if(vargs[2].number().getBoolean() && str.length() > 1) {
				mstruct.clearVector();
				mstruct.resizeVector(str.length(), m_zero);
				if(mstruct.size() < str.length()) return 0;
				for(size_t i = 0; i < str.length(); i++) {
					mstruct[i] = (long int) ((unsigned char) str[i]);
				}
			} else {
				Number nr;
				for(size_t i = 0; i < str.length(); i++) {
					if(i > 0) nr *= 0x100;
					nr += (long int) ((unsigned char) str[i]);
				}
				mstruct = nr;
			}
			break;
		}
		case 1: {}
		case 2: {
			mstruct.clear();
			for(size_t i = 0; i < str.length(); i++) {
				long int c = (unsigned char) str[i];
				if((c & 0x80) != 0) {
					if(c<0xe0) {
						i++;
						if(i >= str.length()) return false;
						c = ((c & 0x1f) << 6) | (((unsigned char) str[i]) & 0x3f);
					} else if(c<0xf0) {
						i++;
						if(i + 1 >= str.length()) return false;
						c = (((c & 0xf) << 12) | ((((unsigned char) str[i]) & 0x3f) << 6)|(((unsigned char) str[i + 1]) & 0x3f));
						i++;
					} else {
						i++;
						if(i + 2 >= str.length()) return false;
						c = ((c & 7) << 18) | ((((unsigned char) str[i]) & 0x3f) << 12) | ((((unsigned char) str[i + 1]) & 0x3f) << 6) | (((unsigned char) str[i + 2]) & 0x3f);
						i += 2;
					}
				}
				long int c_low = -1;
				if(i_encoding == 1) {
					if(c >= 0x10000L) {
						c -= 0x10000L;
						c_low = c % 0x400;
						c_low += 0xDC00;
						c /= 0x400;
						c += 0xD800;
					}
				}
				if(vargs[2].number().getBoolean()) {
					if(mstruct.isZero() && c_low < 0) {
						mstruct.set(c, 1L, 0L);
					} else if(mstruct.isVector()) {
						mstruct.addChild(MathStructure(c, 1L, 0L));
					} else {
						mstruct.transform(STRUCT_VECTOR, MathStructure(c, 1L, 0L));
					}
					if(c_low >= 0) mstruct.addChild(MathStructure(c_low, 1L, 0L));
				} else if(i_encoding == 1) {
					if(i > 0) mstruct.number() *= 0x10000L;
					if(c_low < 0) {
						mstruct.number() += c;
					} else {
						mstruct.number() += c;
						mstruct.number() *= 0x10000L;
						mstruct.number() += c_low;
					}
				} else {
					if(i > 0) {
						mstruct.number() *= 0x10000L;
						mstruct.number() *= 0x10000L;
					}
					mstruct.number() += c;
				}
			}
			break;
		}
		default: {return false;}
	}
	return 1;
}
CharFunction::CharFunction() : MathFunction("char", 1) {
	IntegerArgument *arg = new IntegerArgument();
	Number fr(32, 1, 0);
	arg->setMin(&fr);
	fr.set(0x10ffff, 1, 0);
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
int CharFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {

	long int v = vargs[0].number().lintValue();
	string str;
	if(v <= 0x7f) {
		str = (char) v;
	} else if(v <= 0x7ff) {
		str = (char) ((v >> 6) | 0xc0);
		str += (char) ((v & 0x3f) | 0x80);
	} else if((v <= 0xd7ff || (0xe000 <= v && v <= 0xffff))) {
		str = (char) ((v >> 12) | 0xe0);
		str += (char) (((v >> 6) & 0x3f) | 0x80);
		str += (char) ((v & 0x3f) | 0x80);
	} else if(0xffff < v && v <= 0x10ffff) {
		str = (char) ((v >> 18) | 0xf0);
		str += (char) (((v >> 12) & 0x3f) | 0x80);
		str += (char) (((v >> 6) & 0x3f) | 0x80);
		str += (char) ((v & 0x3f) | 0x80);
	} else {
		return 0;
	}
	mstruct = str;
	return 1;

}

void string_print_set(MathStructure &m) {
	if(m.isVector()) {
		for(size_t i = 0; i < m.size(); i++) {
			string_print_set(m[i]);
		}
	} else {
		m.set(m.print(), true, true);
	}
}

StringFunction::StringFunction() : MathFunction("string", 1, -1) {}
int StringFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) return 0;
		mstruct[i].eval(eo);
	}
	if(mstruct.size() == 1 && mstruct[0].isVector()) {
		mstruct.setToChild(1, true);
	}
	string_print_set(mstruct);
	return 1;
}
CharactersFunction::CharactersFunction() : MathFunction("characters", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int CharactersFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.clearVector();
	for(size_t i = 0; i < vargs[0].symbol().length(); i++) {
		if(CALCULATOR->aborted()) return false;
		string str;
		str += vargs[0].symbol()[i];
		if((unsigned char) str[0] > 0xC0) {
			while(i + 1 < vargs[0].symbol().length() && (signed char) vargs[0].symbol()[i + 1] < 0) {
				i++;
				str += vargs[0].symbol()[i];
			}
		}
		mstruct.addChild_nocopy(new MathStructure(str, true));
	}
	return 1;
}
ConcatenateFunction::ConcatenateFunction() : MathFunction("concatenate", 1, -1) {
	Argument *arg = new TextArgument("", false);
	setArgumentDefinition(1, arg);
	arg = new TextArgument("", false);
	setArgumentDefinition(2, arg);
}
int ConcatenateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure m(vargs);
	for(size_t i = 0; i < m.size(); i++) {
		if(CALCULATOR->aborted()) return 0;
		m[i].eval(eo);
	}
	if(m.size() == 1 && m[0].isVector()) {
		m.setToChild(1, true);
	}
	string str;
	size_t vector_size = (size_t) -1;
	size_t n_vector = 0;
	for(size_t i = 0; i < m.size(); i++) {
		if(m[i].isVector()) {
			n_vector++;
			if(vector_size == (size_t) -1) {
				vector_size = m[i].size();
			} else if(vector_size != m[i].size()) {
				CALCULATOR->error(true, _("Vector size mismatch"), NULL);
				return 0;
			}
		}
	}
	if(n_vector > 0) {
		mstruct.clearVector();
		for(size_t i = 0; i < vector_size; i++) {
			MathStructure *mi = new MathStructure(this, NULL);
			for(size_t i2 = 0; i2 < m.size(); i2++) {
				if(m[i2].isVector()) {
					mi->addChild(m[i2][i]);
				} else {
					mi->addChild(m[i2]);
				}
			}
			mstruct.addChild_nocopy(mi);
		}
		return 1;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(m[i].isSymbolic()) str += m[i].symbol();
		else str += format_and_print(m[i]);
	}
	mstruct.set(str, false, true);
	return 1;
}
LengthFunction::LengthFunction() : MathFunction("len", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int LengthFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) unicode_length(vargs[0].symbol());
	return 1;
}

bool replace_function(MathStructure &m, MathFunction *f1, MathFunction *f2, const EvaluationOptions &eo) {
	bool ret = false;
	if(m.isFunction() && m.function() == f1) {
		m.setFunction(f2);
		while(f2->maxargs() >= 0 && m.size() > (size_t) f2->maxargs()) {
			m.delChild(m.countChildren());
		}
		if(m.size() >= 1) {
			if((f1->getArgumentDefinition(1) && f1->getArgumentDefinition(1)->type() == ARGUMENT_TYPE_ANGLE) && (!f2->getArgumentDefinition(1) || f2->getArgumentDefinition(1)->type() != ARGUMENT_TYPE_ANGLE)) {
				if(m[0].contains(CALCULATOR->getRadUnit(), false, true, true) > 0) m[0] /= CALCULATOR->getRadUnit();
				else if(m[0].contains(CALCULATOR->getDegUnit(), false, true, true) > 0) m[0] /= CALCULATOR->getDegUnit();
				else if(m[0].contains(CALCULATOR->getGraUnit(), false, true, true) > 0) m[0] /= CALCULATOR->getGraUnit();
				else if(CALCULATOR->customAngleUnit() && m[0].contains(CALCULATOR->customAngleUnit(), false, true, true) > 0) m[0] /= CALCULATOR->customAngleUnit();
			} else if((f2->getArgumentDefinition(1) && f2->getArgumentDefinition(1)->type() == ARGUMENT_TYPE_ANGLE) && (!f1->getArgumentDefinition(1) || f1->getArgumentDefinition(1)->type() != ARGUMENT_TYPE_ANGLE)) {
				Unit *u = default_angle_unit(eo);
				if(u) m[0] *= u;
			}
		}
		ret = true;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_function(m[i], f1, f2, eo)) ret = true;
	}
	return ret;
}
ReplaceFunction::ReplaceFunction() : MathFunction("replace", 3, 4) {
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(4, "0");
}
int ReplaceFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	bool b_evaled = false;
	bool replace_in_variables = vargs[1].containsUnknowns();
	if(vargs[3].number().getBoolean()) {mstruct.eval(eo); b_evaled = true;}
	if(vargs[1].isVector() && !vargs[2].isVector() && !vargs[2].representsScalar()) {
		MathStructure meval(vargs[2]);
		CALCULATOR->beginTemporaryStopMessages();
		meval.eval(eo);
		if(meval.isVector() && vargs[1].size() == meval.size()) {
			CALCULATOR->endTemporaryStopMessages(true);
			for(size_t i = 0; i < vargs[1].size(); i++) {
				if(vargs[1][i].isFunction() && meval[i].isFunction() && vargs[1][i].size() == 0 && meval[i].size() == 0) {
					if(!replace_function(mstruct, vargs[1][i].function(), meval[i].function(), eo)) CALCULATOR->error(false, _("Original value (%s) was not found."), (vargs[1][i].function()->name() + "()").c_str(), NULL);
				} else if(meval[i].containsInterval(true, false, false, 0, true)) {
					MathStructure mv(meval[i]);
					replace_f_interval(mv, eo);
					replace_intervals_f(mv);
					if(!mstruct.replace(vargs[1][i], mv, false, false, replace_in_variables)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(vargs[1][i]).c_str(), NULL);
				} else {
					if(!mstruct.replace(vargs[1][i], meval[i], false, false, replace_in_variables)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(vargs[1][i]).c_str(), NULL);
				}
			}
			return 1;
		}
		CALCULATOR->endTemporaryStopMessages(false);
	}
	if(vargs[1].isVector() && vargs[2].isVector() && vargs[1].size() == vargs[2].size()) {
		for(size_t i = 0; i < vargs[1].size(); i++) {
			if(vargs[1][i].isFunction() && vargs[2][i].isFunction() && vargs[1][i].size() == 0 && vargs[2][i].size() == 0) {
				if(!replace_function(mstruct, vargs[1][i].function(), vargs[2][i].function(), eo)) CALCULATOR->error(false, _("Original value (%s) was not found."), (vargs[1][i].function()->name() + "()").c_str(), NULL);
			} else if(vargs[2][i].containsInterval(true, false, false, 0, true)) {
				MathStructure mv(vargs[2][i]);
				replace_f_interval(mv, eo);
				replace_intervals_f(mv);
				if(!mstruct.replace(vargs[1][i], mv, false, false, replace_in_variables)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(vargs[1][i]).c_str(), NULL);
			} else {
				if(!mstruct.replace(vargs[1][i], vargs[2][i], false, false, replace_in_variables)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(vargs[1][i]).c_str(), NULL);
			}
		}
	} else {
		while(true) {
			if(vargs[1].isFunction() && vargs[2].isFunction() && vargs[1].size() == 0 && vargs[2].size() == 0) {
				if(replace_function(mstruct, vargs[1].function(), vargs[2].function(), eo)) break;
			} else if(vargs[2].containsInterval(true, false, false, 0, true)) {
				MathStructure mv(vargs[2]);
				replace_f_interval(mv, eo);
				replace_intervals_f(mv);
				if(mstruct.replace(vargs[1], mv, false, false, replace_in_variables)) break;
			} else {
				if(mstruct.replace(vargs[1], vargs[2], false, false, replace_in_variables)) break;
			}
			if(b_evaled) {
				CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(vargs[1]).c_str(), NULL);
				break;
			} else {
				mstruct.eval();
				b_evaled = true;
			}
		}
	}
	return 1;
}
void remove_nounit(MathStructure &mstruct) {
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS && mstruct.size() == 1) {
		mstruct.setToChild(1, true);
	}
	if(mstruct.isMultiplication() || mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			remove_nounit(mstruct[i]);
		}
	}
}
StripUnitsFunction::StripUnitsFunction() : MathFunction("nounit", 1) {}
int StripUnitsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	remove_nounit(mstruct);
	mstruct.removeType(STRUCT_UNIT);
	if(mstruct.containsType(STRUCT_UNIT, false, true, true) == 0) return 1;
	if(mstruct.isMultiplication() || mstruct.isAddition()) {
		MathStructure *mleft = NULL;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].containsType(STRUCT_UNIT, false, true, true) == 0) {
				mstruct[i].ref();
				if(mleft) {
					if(mstruct.isMultiplication()) mleft->multiply_nocopy(&mstruct[i], true);
					else mleft->add_nocopy(&mstruct[i], true);
				} else {
					mleft = &mstruct[i];
				}
				mstruct.delChild(i + 1);
			}
		}
		if(mleft) {
			if(mstruct.size() == 0) {
				mstruct.set_nocopy(*mleft);
				mleft->unref();
			} else {
				bool b_multi = mstruct.isMultiplication();
				if(mstruct.size() == 1) {
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
				} else {
					mstruct.transform(this);
				}
				if(b_multi) mstruct.multiply_nocopy(mleft, true);
				else mstruct.add_nocopy(mleft, true);
			}
			return 1;
		}
	}
	EvaluationOptions eo2 = eo;
	eo2.sync_units = false;
	eo2.keep_prefixes = true;
	mstruct.eval(eo2);
	remove_nounit(mstruct);
	mstruct.removeType(STRUCT_UNIT);
	if(mstruct.containsType(STRUCT_UNIT, false, true, true) == 0) return 1;
	if(mstruct.isMultiplication() || mstruct.isAddition()) {
		MathStructure *mleft = NULL;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].containsType(STRUCT_UNIT, false, true, true) == 0) {
				mstruct[i].ref();
				if(mleft) {
					if(mstruct.isMultiplication()) mleft->multiply_nocopy(&mstruct[i], true);
					else mleft->add_nocopy(&mstruct[i], true);
				} else {
					mleft = &mstruct[i];
				}
				mstruct.delChild(i + 1);
			}
		}
		if(mleft) {
			if(mstruct.size() == 0) {
				mstruct.set_nocopy(*mleft);
				mleft->unref();
			} else {
				bool b_multi = mstruct.isMultiplication();
				if(mstruct.size() == 1) {
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
				} else {
					mstruct.transform(this);
				}
				if(b_multi) mstruct.multiply_nocopy(mleft, true);
				else mstruct.add_nocopy(mleft, true);
			}
			return 1;
		}
	}
	return -1;
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
		if(mprocess.isFunction() && mprocess.function()->id() == FUNCTION_ID_ELEMENT && mprocess.size() >= 2 && mprocess[0] == vargs[8]) {
			bool b = csum_replace(mprocess[1], mstruct, vargs, index, eo2);
			mprocess[1].eval(eo2);
			if(mprocess[1].isNumber() && mprocess[1].number().isInteger() && mprocess[1].number().isPositive() && mprocess[1].number().isLessThanOrEqualTo(vargs[6].size())) {
				mprocess = vargs[6][mprocess[1].number().intValue() - 1];
				return true;
			}
			return csum_replace(mprocess[0], mstruct, vargs, index, eo2) || b;
		} else if(mprocess.isFunction() && mprocess.function()->id() == FUNCTION_ID_COMPONENT && mprocess.size() == 2 && mprocess[1] == vargs[8]) {
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
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SINT);
	setArgumentDefinition(1, arg); //begin
	arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT);
	arg->setHandleVector(false);
	setArgumentDefinition(2, arg); //end
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
	if(!check_recursive_depth(mstruct)) return 0;
	return 1;

}

void fix_user_function_expression(string &str, const EvaluationOptions &eo) {
	gsub("{", "\a", str);
	gsub("}", "\b", str);
	ParseOptions pa = eo.parse_options; pa.base = 10;
	str = CALCULATOR->unlocalizeExpression(str, pa);
	if(str.empty()) return;
	size_t i = 0;
	bool b = false;
	while(true) {
		i = str.find("\\", i);
		if(i == string::npos || i == str.length() - 1) break;
		if((str[i + 1] >= 'a' && str[i + 1] <= 'z') || (str[i + 1] >= 'A' && str[i + 1] <= 'Z') || (str[i + 1] >= '1' && str[i + 1] <= '9')) {
			b = true;
			break;
		}
		i++;
	}
	CALCULATOR->parseSigns(str);
	if(!b) {
		bool in_cit1 = false, in_cit2 = false;
		for(i = 0; i < str.length(); i++) {
			if(!in_cit2 && str[i] == '\"') {
				in_cit1 = !in_cit1;
			} else if(!in_cit1 && str[i] == '\'') {
				in_cit2 = !in_cit2;
			} else if(!in_cit1 && !in_cit2 && (str[i] == 'x' || str[i] == 'y' || str[i] == 'z' || str[i] == 'X' || str[i] == 'Y' || str[i] == 'Z')) {
				size_t i2 = str.find_last_of(NOT_IN_NAMES NUMBERS, i);
				size_t i3 = str.find_first_of(NOT_IN_NAMES NUMBERS, i);
				if(i2 == string::npos) i2 = 0;
				else i2++;
				if(i3 == string::npos) i3 = str.length();
				size_t i4 = i2;
				if(i4 > 0) {
					i4 = str.find_last_of(NOT_IN_NAMES, i4);
					if(i4 == string::npos) i4 = 0;
					else i4++;
				}
				size_t i5 = i3;
				if(i5 < str.length()) {
					i5 = str.find_first_of(NOT_IN_NAMES, i5 - 1);
					if(i5 == string::npos) i5 = str.length();
				}
				if((i2 == i3 - 1 || !CALCULATOR->getActiveExpressionItem(str.substr(i2, i3 - i2))) && ((i4 == i2 && i5 == i3) || !CALCULATOR->getActiveExpressionItem(str.substr(i4, i5 - i4)))) {
					str.insert(i, 1, '\\');
					i++;
				} else {
					i = i5 - 1;
				}
			}
		}
	}
	gsub("\a", "{", str);
	gsub("\b", "}", str);
}

FunctionFunction::FunctionFunction() : MathFunction("function", 1, -1) {
	Argument *arg = new TextArgument("", false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int FunctionFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	string str;
	if(vargs[0].isSymbolic()) {
		str = vargs[0].symbol();
	} else {
		MathStructure meval = vargs[0];
		CALCULATOR->beginTemporaryStopMessages();
		meval.eval(eo);
		if(meval.isSymbolic()) {
			CALCULATOR->endTemporaryStopMessages(true);
			str = meval.symbol();
		} else {
			str = vargs[0].print(CALCULATOR->save_printoptions);
			CALCULATOR->endTemporaryStopMessages();
		}
	}
	fix_user_function_expression(str, eo);
	UserFunction f = new UserFunction("", "Generated MathFunction", str);
	MathStructure args = vargs;
	args.delChild(1);
	mstruct = f.MathFunction::calculate(args, eo);
	if(mstruct.isFunction() && mstruct.function() == &f) mstruct.setUndefined();
	return 1;
}

REPRESENTS_FUNCTION(RepresentsIntegerFunction, representsInteger)
REPRESENTS_FUNCTION(RepresentsRealFunction, representsReal)
REPRESENTS_FUNCTION(RepresentsRationalFunction, representsRational)
REPRESENTS_FUNCTION(RepresentsNumberFunction, representsNumber)

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
bool replace_variable(MathStructure &m, KnownVariable *v) {
	if(m.isVariable()) {
		if(m.variable() == v) {
			m.set(v->get(), true);
			return true;
		} else if(m.variable()->isKnown() && m.contains(v, true, true)) {
			m.set(((KnownVariable*) m.variable())->get(), true);
			replace_variable(m, v);
			return true;
		}
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_variable(m[i], v)) {
			b_ret = true;
			m.childUpdated(i + 1);
		}
	}
	return b_ret;
}
SaveFunction::SaveFunction() : MathFunction("save", 2, 5) {
	setArgumentDefinition(2, new TextArgument());
	setArgumentDefinition(3, new TextArgument());
	setArgumentDefinition(4, new TextArgument());
	setArgumentDefinition(5, new BooleanArgument());
	setDefaultValue(3, CALCULATOR->temporaryCategory());
	setDefaultValue(4, "\"\"");
	setDefaultValue(5, "0");
}
int SaveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	if(vargs[4].number().getBoolean()) mstruct.eval(eo);
	size_t i = vargs[1].symbol().find(LEFT_PARENTHESIS, 1);
	size_t i2 = string::npos;
	if(i != string::npos) i2 = vargs[1].symbol().find_last_not_of(SPACES, i - 1);
	if(i2 != string::npos) {
		string name = vargs[1].symbol().substr(0, i2 + 1);
		if(!CALCULATOR->functionNameIsValid(name)) {
			CALCULATOR->error(true, _("Invalid function name (%s)."), name.c_str(), NULL);
			if(vargs[4].number().getBoolean()) return -1;
			return 0;
		}
		string sarg = vargs[1].symbol().substr(i, vargs[1].symbol().length() - i);
		gsub(";", ",", sarg);
		sarg.insert(0, CALCULATOR->getFunctionById(FUNCTION_ID_VECTOR)->referenceName());
		MathStructure marg;
		CALCULATOR->parse(&marg, sarg, eo.parse_options);
		if(marg.size() == 0) {
			MathStructure m("x", true);
			if(!mstruct.contains(m, true) && mstruct.contains(CALCULATOR->getVariableById(VARIABLE_ID_X), true)) {
				if(mstruct.replace(CALCULATOR->getVariableById(VARIABLE_ID_X), m)) {
					m.set("y", false, true);
					if(mstruct.replace(CALCULATOR->getVariableById(VARIABLE_ID_Y), m)) {
						m.set("z", false, true);
						mstruct.replace(CALCULATOR->getVariableById(VARIABLE_ID_Z), m);
					}
				}
			}
		} else {
			string sarg = "x";
			MathStructure m(sarg, true);
			for(i = 0; i < marg.size(); i++) {
				mstruct.replace(marg[i], m);
				if(sarg[0] == 'z') sarg[0] = 'a';
				else sarg[0]++;
				m.set(sarg, false, true);
			}
		}
		string expr = mstruct.print(CALCULATOR->save_printoptions);
		char carg = 'x';
		string sarg_new = "\\x", sarg2 = "\'x\'"; sarg = "\"x\"";
		for(i = 0; i < marg.size() || expr.find(sarg) != string::npos || expr.find(sarg2) != string::npos; i++) {
			gsub(sarg, sarg_new, expr);
			gsub(sarg2, sarg_new, expr);
			if(carg == 'z') carg = 'a';
			else carg++;
			sarg[1] = carg;
			sarg2[1] = carg;
			sarg_new[1] = carg;
		}
		if(CALCULATOR->hasToExpression(expr)) CALCULATOR->error(false, _("Conversion (using \"to\") is not supported in functions."), NULL);
		if(CALCULATOR->functionNameTaken(name)) {
			MathFunction *f = CALCULATOR->getActiveFunction(name, true);
			if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
				if(!vargs[2].symbol().empty()) f->setCategory(vargs[2].symbol());
				if(!vargs[3].symbol().empty()) f->setTitle(vargs[3].symbol());
				((UserFunction*) f)->setFormula(expr, marg.size() == 0 ? -1 : marg.size());
				if(f->countNames() == 0) {
					ExpressionName ename(name);
					ename.reference = true;
					f->setName(ename, 1);
				} else {
					f->setName(name, 1);
				}
			} else {
				CALCULATOR->error(false, MESSAGE_CATEGORY_GLOBAL_OBJECT_DEACTIVATED, _("A global function was deactivated. It will be restored after the new function has been removed."), NULL);
				CALCULATOR->addFunction(new UserFunction(vargs[2].symbol(), name, expr, true, marg.size() == 0 ? -1 : marg.size(), vargs[3].symbol()))->setChanged(true);
			}
		} else {
			CALCULATOR->addFunction(new UserFunction(vargs[2].symbol(), name, expr, true, marg.size() == 0 ? -1 : marg.size(), vargs[3].symbol()))->setChanged(true);
		}
		mstruct = expr;
		CALCULATOR->saveFunctionCalled();
		return 1;
	}
	if(vargs[4].number().getBoolean()) {
		if(mstruct.isComparison() && mstruct.comparisonType() == COMPARISON_EQUALS && (mstruct[0].isSymbolic() || (mstruct[0].isVariable() && !mstruct[0].variable()->isKnown()))) {
			mstruct.setToChild(2, true);
		} else if(mstruct.isLogicalAnd() && mstruct.size() > 0 && mstruct[0].isComparison() && mstruct[0].comparisonType() == COMPARISON_EQUALS && (mstruct[0][0].isSymbolic() || (mstruct[0][0].isVariable() && !mstruct[0][0].variable()->isKnown()))) {
			bool b = true;
			for(size_t i = 1; i < mstruct.size(); i++) {
				if(!mstruct[i].isComparison() || mstruct[i].comparisonType() == COMPARISON_EQUALS) {
					b = false;
					break;
				}
			}
			if(b) {
				mstruct.setToChild(1, true);
				mstruct.setToChild(2, true);
			}
		}
	}
	i = vargs[1].symbol().find(LEFT_VECTOR_WRAP, 1);
	i2 = string::npos;
	if(i != string::npos) i2 = vargs[1].symbol().find_last_not_of(SPACES, i - 1);
	if(i2 != string::npos) {
		string name = vargs[1].symbol().substr(0, i2 + 1);
		Variable *v = CALCULATOR->getActiveVariable(name);
		if(!v || !v->isLocal() || !v->isKnown() || !((KnownVariable*) v)->get().isVector()) {
			CALCULATOR->error(true, _("Matrix/vector (%s) not found."), name.c_str(), NULL);
			if(vargs[4].number().getBoolean()) return -1;
			return 0;
		}
		i2 = vargs[1].symbol().rfind(RIGHT_VECTOR_WRAP);
		string index, index2;
		if(i2 == string::npos || i2 < i) index = vargs[1].symbol().substr(i + 1);
		else index = vargs[1].symbol().substr(i + 1, i2 - (i + 1));
		size_t i3 = index.find(RIGHT_VECTOR_WRAP);
		if(i3 != string::npos && i3 != index.length() - 1) {
			size_t i4 = index.find(LEFT_VECTOR_WRAP);
			if(i4 != string::npos && i4 == i3 + 1 && index.find_first_of(VECTOR_WRAPS, i4 + 1) == string::npos) {
				if(i4 != index.length() - 1) index2 = index.substr(i4 + 1);
				index = index.substr(0, i3);
			}
		}
		MathStructure mindex, mindex2;
		gsub(";", COMMA, index);
		ParseOptions po = eo.parse_options;
		po.base = 10;
		CALCULATOR->parse(&mindex, index, po);
		mindex.eval(eo);
		if(!index2.empty()) {
			CALCULATOR->parse(&mindex2, index2, po);
			mindex2.eval(eo);
		}
		mstruct.transformById(FUNCTION_ID_REPLACE_PART);
		mstruct.insertChild(((KnownVariable*) v)->get(), 1);
		if(!index2.empty()) {
			mstruct.addChild(mindex);
			mstruct.addChild(mindex2);
		} else if(mindex.isVector()) {
			for(size_t i = 0; i < mindex.size(); i++) {
				mstruct.addChild(mindex[i]);
			}
		} else {
			if(!((KnownVariable*) v)->get().isMatrix()) mstruct.addChild(m_one);
			mstruct.addChild(mindex);
		}
		while(mstruct.size() < 6) mstruct.addChild(m_zero);
		mstruct.calculateFunctions(eo, false);
		if(mstruct.isVector()) {
			if(mstruct.isMatrix() && !((KnownVariable*) v)->get().isMatrix() && mstruct.size() == 1) ((KnownVariable*) v)->set(mstruct[0]);
			else ((KnownVariable*) v)->set(mstruct);
		} else {
			return 0;
		}
		CALCULATOR->saveFunctionCalled();
		return 1;
	}
	if(!CALCULATOR->variableNameIsValid(vargs[1].symbol())) {
		CALCULATOR->error(true, _("Invalid variable name (%s)."), vargs[1].symbol().c_str(), NULL);
		if(vargs[4].number().getBoolean()) return -1;
		return 0;
	}
	if(CALCULATOR->variableNameTaken(vargs[1].symbol())) {
		Variable *v = CALCULATOR->getActiveVariable(vargs[1].symbol(), true);
		if(v && v->isLocal() && v->isKnown()) {
			if(!vargs[2].symbol().empty()) v->setCategory(vargs[2].symbol());
			if(!vargs[3].symbol().empty()) v->setTitle(vargs[3].symbol());
			replace_variable(mstruct, (KnownVariable*) v);
			((KnownVariable*) v)->set(mstruct);
			if(v->countNames() == 0) {
				ExpressionName ename(vargs[1].symbol());
				ename.reference = true;
				v->setName(ename, 1);
			} else {
				v->setName(vargs[1].symbol(), 1);
			}
		} else {
			CALCULATOR->error(false, MESSAGE_CATEGORY_GLOBAL_OBJECT_DEACTIVATED, _("A global unit or variable was deactivated. It will be restored after the new variable has been removed."), NULL);
			CALCULATOR->addVariable(new KnownVariable(vargs[2].symbol(), vargs[1].symbol(), mstruct, vargs[3].symbol()))->setChanged(true);
		}
	} else {
		CALCULATOR->addVariable(new KnownVariable(vargs[2].symbol(), vargs[1].symbol(), mstruct, vargs[3].symbol()))->setChanged(true);
	}
	CALCULATOR->saveFunctionCalled();
	return 1;
}

RegisterFunction::RegisterFunction() : MathFunction("register", 1) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE));
}
int RegisterFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].number().isGreaterThan(CALCULATOR->RPNStackSize())) {
		CALCULATOR->error(false, _("Register %s does not exist. Returning zero."), format_and_print(vargs[0]).c_str(), NULL);
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

CommandFunction::CommandFunction() : MathFunction("command", 1, -1) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new Argument());
}
int CommandFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
#ifndef DISABLE_INSECURE
#	ifdef HAVE_GNUPLOT_CALL

	FILE *pipe = NULL;
	string commandline = vargs[0].symbol();
	remove_blank_ends(commandline);
	if(commandline.empty()) {
		CALCULATOR->error(true, _("Failed to run external command (%s)."), commandline.c_str(), NULL);
		return 0;
	}
	bool quoted = commandline.length() >= 2 && (commandline[0] == '\"' || commandline[0] == '\'') && commandline.find(commandline[0], 1) == commandline.length() - 1;
	if(!quoted) {
		if(commandline.find("\'") != string::npos) {
			CALCULATOR->error(true, _("Failed to run external command (%s)."), commandline.c_str(), NULL);
			return 0;
		}
		commandline.insert(0, "\'");
		commandline += "\'";
	}
	string cmd;
	size_t pos = commandline.find_last_of("/\\");
	if(pos == string::npos) pos = 0;
	cmd = commandline.substr(pos + 1, commandline.length() - (pos + 1) - 1);
	if(cmd.empty() || cmd == "rm" || cmd == "wget" || cmd == "curl" || cmd == "exec" || cmd == "rmdir" || cmd == "su" || cmd == "sudo" || cmd.find("run") == 0 || cmd.find("python") == 0 || cmd.find("perl") == 0 || cmd.find("sh", cmd.length() - 2) != string::npos || cmd == "fdisk" || cmd.find("-open") != string::npos || cmd.find("-launch") != string::npos || cmd.find("terminal") != string::npos || cmd.find("rxvt") != string::npos || (cmd.length() >= 4 && cmd.find("term", cmd.length() - 4) != string::npos) || cmd.find("shell") != string::npos || cmd.find("command") != string::npos) {
		CALCULATOR->error(true, _("Failed to run external command (%s)."), commandline.c_str(), NULL);
		return 0;
	}
	for(size_t i = 1; i < vargs.size(); i++) {
		commandline += " ";
		if(vargs[i].isSymbolic()) {
			commandline += "\'";
			commandline += vargs[i].symbol();
			commandline += "\'";
		} else {
			MathStructure m(vargs[i]);
			m.eval(eo);
			commandline += "\'";
			commandline += m.print(CALCULATOR->save_printoptions);
			commandline += "\'";
		}
	}

#ifdef _WIN32
	pipe = _popen((commandline + " 2>nul").c_str(), "r");
#else
	pipe = popen((commandline + " 2>/dev/null").c_str(), "r");
#endif

	if(!pipe) {
		CALCULATOR->error(true, _("Failed to run external command (%s)."), commandline.c_str(), NULL);
		return 0;
	}

	char buffer[1000];
	string output;
	while(fgets(buffer, sizeof(buffer), pipe) != NULL) {
		output += buffer;
	}

#ifdef _WIN32
	if(_pclose(pipe) > 0 && output.empty()) {
#else
	if(pclose(pipe) > 0 && output.empty()) {
#endif
		CALCULATOR->error(true, _("Failed to run external command (%s)."), commandline.c_str(), NULL);
		return 0;
	}

	ParseOptions po;
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->parse(&mstruct, output, po);
	vector<CalculatorMessage> blocked_messages;
	CALCULATOR->endTemporaryStopMessages(false, &blocked_messages);
	bool b_error = blocked_messages.size() > 5;
	for(size_t i = 0; !b_error && i < blocked_messages.size(); i++) {
		if(blocked_messages[i].type() == MESSAGE_ERROR) b_error = true;
	}
	if(!b_error) {
		long long int n = mstruct.countTotalChildren(false);
		if(n > 1000) {
			if(mstruct.isMatrix()) b_error = n > ((long long int) mstruct.rows()) * ((long long int) mstruct.columns()) * 10;
			else if(mstruct.isVector()) b_error = n > ((long long int) mstruct.size()) * 10;
			else b_error = true;
		}
	}
	if(b_error) {
		size_t i = output.find("\n");
		if(i != string::npos && i > 0 && i < output.length() - 1) output.insert(0, "\n");
		CALCULATOR->error(true, _("Parsing of command output failed: %s"), output.c_str(), NULL);
		return 0;
	}
	CALCULATOR->addMessages(&blocked_messages);

	return 1;

#	else
	CALCULATOR->error(true, _("%s is disabled when %s is compiled with \"%s\" configure option."), (name() + "()").c_str(), "libqalculate", "--without-gnuplot-call", NULL);
	return 0;
#	endif
#else
	CALCULATOR->error(true, _("%s is disabled when %s is compiled with \"%s\" configure option."), (name() + "()").c_str(), "libqalculate", "--disable-insecure", NULL);
	return 0;
#endif
}

#define PLOT_OPTION_LOCALE_SEPARATOR " / "
#define PLOT_OPTION_SEPARATOR "\n"
#define LIST_PLOT_OPTION_1(x) if(strcmp(_c("plot", x), x)) {str += _c("plot", x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x;
#define LIST_PLOT_OPTION_2(x, y) str += _c("plot", x); str += PLOT_OPTION_LOCALE_SEPARATOR; str += y;
#define LIST_PLOT_OPTION(x) LIST_PLOT_OPTION_1(x) str += PLOT_OPTION_SEPARATOR;
#define LIST_PLOT_OPTION_ALT(x, y) LIST_PLOT_OPTION_2(x, y) str += PLOT_OPTION_SEPARATOR;
#define LIST_PLOT_OPTION_LAST(x) LIST_PLOT_OPTION_1(x)
#define LIST_PLOT_OPTION_WV(x) LIST_PLOT_OPTION_1(x)
#define LIST_PLOT_OPTION_ALT_WV(x, y) LIST_PLOT_OPTION_2(x, y)
#define LIST_PLOT_VALUE(y, x) if(strcmp(x, _c(y, x))) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ", ";
#define LIST_PLOT_VALUE_FIRST(y, x) str += " ("; if(strcmp(x, _c(y, x))) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ", ";
#define LIST_PLOT_VALUE_LAST(y, x) if(strcmp(x, _c(y, x))) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ")\n";
#define LIST_PLOT_OPTION_ALT_VALUES(x, y, z) LIST_PLOT_OPTION_2(x, y) str += " ("; str += z; str += ")"; str += PLOT_OPTION_SEPARATOR;
#define LIST_PLOT_OPTION_VALUES(x, y) LIST_PLOT_OPTION_1(x) str += " ("; str += y; str += ")"; str += PLOT_OPTION_SEPARATOR;

PlotFunction::PlotFunction() : MathFunction("plot", 1, -1) {
	NumberArgument *arg = new NumberArgument();
	arg->setComplexAllowed(false);
	arg->setHandleVector(false);
	setArgumentDefinition(2, arg);
	setDefaultValue(2, "0");
	arg = new NumberArgument();
	arg->setHandleVector(false);
	arg->setComplexAllowed(false);
	setArgumentDefinition(3, arg);
	setDefaultValue(3, "10");
	setArgumentDefinition(4, new TextArgument());
	setCondition("\\y < \\z");
	string str = _("Plots one or more expressions or vectors. Use a vector for the first argument to plot multiple series. Only the first argument is used for vector series. It is also possible to plot a matrix where each row is a pair of x and y values.");
	str += "\n\n";
	str += _("Additional arguments specify various plot options. Enter the name of the option and the desired value, either separated by space or as separate arguments. For most options, the value can be omitted to enable a default active value. For options with named values, the option name can be omitted (otherwise the value can be replaced by an integer, representing the index of the value starting from zero). If the first option specified is a numerical value, this is interpreted as either sampling rate (for integers > 10) or step value.");
	str += "\n\n";
	str += _("List of options:");
	str += "\n";
	LIST_PLOT_OPTION("samples");
	LIST_PLOT_OPTION("step");
	LIST_PLOT_OPTION_ALT_VALUES("variable", "var", "x, y, z, ...");
	LIST_PLOT_OPTION_WV("style"); LIST_PLOT_VALUE_FIRST("plot style", "lines"); LIST_PLOT_VALUE("plot style", "points"); LIST_PLOT_VALUE("plot style", "linespoints"); LIST_PLOT_VALUE("plot style", "boxes"); LIST_PLOT_VALUE("plot style", "histogram"); LIST_PLOT_VALUE("plot style", "steps"); LIST_PLOT_VALUE("plot style", "candlesticks"); LIST_PLOT_VALUE("plot style", "dots"); LIST_PLOT_VALUE_LAST("plot", "polar");
	LIST_PLOT_OPTION_WV("smooth"); LIST_PLOT_VALUE_FIRST("plot smoothing", "none"); LIST_PLOT_VALUE("plot smoothing", "splines"); LIST_PLOT_VALUE_LAST("plot smoothing", "bezier");
	LIST_PLOT_OPTION("ymin");
	LIST_PLOT_OPTION("ymax");
	LIST_PLOT_OPTION("xlog");
	LIST_PLOT_OPTION("ylog");
	LIST_PLOT_OPTION_VALUES("complex", "0, 1");
	LIST_PLOT_OPTION("grid");
	LIST_PLOT_OPTION_ALT("linewidth", "lw");
	LIST_PLOT_OPTION_ALT_WV("legend", "key"); LIST_PLOT_VALUE_FIRST("plot legend", "none"); LIST_PLOT_VALUE("plot legend", "top-left"); LIST_PLOT_VALUE("plot legend", "top-right"); LIST_PLOT_VALUE("plot legend", "bottom-left"); LIST_PLOT_VALUE("plot legend", "bottom-right"); LIST_PLOT_VALUE("plot legend", "below"); LIST_PLOT_VALUE_LAST("plot legend", "outside");
	LIST_PLOT_OPTION("title");
	LIST_PLOT_OPTION("xlabel");
	LIST_PLOT_OPTION_LAST("ylabel");
	setDescription(str);
}

int PlotFunction::parse(MathStructure &mstruct, const std::string &eq, const ParseOptions &po) {
	int ret = MathFunction::parse(mstruct, eq, po);
	if(mstruct.size() > 0 && mstruct[0].isComparison() && mstruct[0].comparisonType() == COMPARISON_EQUALS && mstruct[0][0].isVariable() && mstruct[0][0].variable() == CALCULATOR->getVariableById(VARIABLE_ID_Y) && mstruct[0][1].contains(CALCULATOR->getVariableById(VARIABLE_ID_X))) {
		mstruct[0].setToChild(2, true);
	}
	return ret;
}

int PlotFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
#ifndef HAVE_GNUPLOT_CALL
#	ifndef HAVE_BYO_GNUPLOT
	CALCULATOR->error(true, _("%s is disabled when %s is compiled with \"%s\" configure option."), (name() + "()").c_str(), "libqalculate", "--without-gnuplot-call", NULL);
	return 0;
#	endif
#endif
	if(!CALCULATOR->canPlot()) {
		CALCULATOR->error(true, _("Gnuplot was not found"), NULL);
		return 0;
	}
	EvaluationOptions eo2;
	eo2.parse_options = eo.parse_options;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	eo2.parse_options.read_precision = DONT_READ_PRECISION;
	eo2.interval_calculation = INTERVAL_CALCULATION_NONE;
	eo2.allow_complex = eo.allow_complex;
	bool b_persistent = false;
	PlotParameters param;
	PlotStyle style = PLOT_STYLE_LINES;
	PlotSmoothing smoothing = PLOT_SMOOTHING_NONE;
	MathStructure mstep, mvar(CALCULATOR->getVariableById(VARIABLE_ID_X));
	int i_rate = 1001;
	int i_prev = 0;
	int old_step = 0;
	for(size_t i = 3; i < vargs.size(); i++) {
		string svar = vargs[i].symbol();
		remove_blank_ends(svar);
		gsub(SIGN_MINUS, MINUS, svar);
		string svalue;
		bool b_option = false;
		while(!b_option) {
			b_option = true;
			if(equalsIgnoreCase(svar, "persistent")) {b_persistent = true; i_prev = 5; break;}
			else if(equalsIgnoreCase(svar, "smooth") || equalsIgnoreCase(svar, "smoothing") || equalsIgnoreCase(svar, _c("plot", "smooth"))) {smoothing = PLOT_SMOOTHING_CSPLINES; i_prev = 3; break;}
			else if(equalsIgnoreCase(svar, "style") || equalsIgnoreCase(svar, "type") || equalsIgnoreCase(svar, _c("plot", "style"))) {style = PLOT_STYLE_POINTS; i_prev = 4; break;}
			else if(equalsIgnoreCase(svar, "step") || equalsIgnoreCase(svar, "step size") || equalsIgnoreCase(svar, _c("plot", "step"))) {mstep = m_one; i_prev = 2; break;}
			else if(equalsIgnoreCase(svar, "rate") || equalsIgnoreCase(svar, "samples") || equalsIgnoreCase(svar, _c("plot", "samples"))) {mstep.clear(); i_rate = 1001; i_prev = 1; break;}
			else if(equalsIgnoreCase(svar, "color") || equalsIgnoreCase(svar, _c("plot", "color"))) {param.color = true; i_prev = 11; break;}
			else if(equalsIgnoreCase(svar, "grid") || equalsIgnoreCase(svar, _c("plot", "grid"))) {param.grid = true; i_prev = 13; break;}
			else if(equalsIgnoreCase(svar, "title") || equalsIgnoreCase(svar, _c("plot", "title"))) i_prev = 15;
			else if(equalsIgnoreCase(svar, "legend") || equalsIgnoreCase(svar, "key") || equalsIgnoreCase(svar, _c("plot", "legend"))) {i_prev = 18; param.legend_placement = PLOT_LEGEND_BELOW;}
			else if(equalsIgnoreCase(svar, "ylabel") || equalsIgnoreCase(svar, "y-label") || equalsIgnoreCase(svar, "y label") || equalsIgnoreCase(svar, _c("plot", "ylabel"))) i_prev = 16;
			else if(equalsIgnoreCase(svar, "xlabel") || equalsIgnoreCase(svar, "x-label") || equalsIgnoreCase(svar, "x label") || equalsIgnoreCase(svar, _c("plot", "xlabel"))) i_prev = 17;
			else if(equalsIgnoreCase(svar, "linewidth") || equalsIgnoreCase(svar, "lw") || equalsIgnoreCase(svar, "line width") || equalsIgnoreCase(svar, _c("plot", "linewidth"))) {i_prev = 14; break;}
			else if(equalsIgnoreCase(svar, "mono") || equalsIgnoreCase(svar, "monochrome") || equalsIgnoreCase(svar, _c("plot", "monochrome"))) {param.color = false; i_prev = 12; break;}
			else if(equalsIgnoreCase(svar, "ymin") || equalsIgnoreCase(svar, "y-min") || equalsIgnoreCase(svar, "y min") || equalsIgnoreCase(svar, _c("plot", "ymin"))) i_prev = 7;
			else if(equalsIgnoreCase(svar, "ymax") || equalsIgnoreCase(svar, "y-max") || equalsIgnoreCase(svar, "y max") || equalsIgnoreCase(svar, _c("plot", "ymax"))) i_prev = 8;
			else if(equalsIgnoreCase(svar, "ylog") || equalsIgnoreCase(svar, "logscale y") || equalsIgnoreCase(svar, "y-log") || equalsIgnoreCase(svar, "y log") || equalsIgnoreCase(svar, _c("plot", "ylog"))) {param.y_log = true; i_prev = 9; break;}
			else if(equalsIgnoreCase(svar, "xlog") || equalsIgnoreCase(svar, "logscale x") || equalsIgnoreCase(svar, "x-log") || equalsIgnoreCase(svar, "x log") || equalsIgnoreCase(svar, _c("plot", "xlog"))) {param.x_log = true; i_prev = 10; break;}
			else if(equalsIgnoreCase(svar, "logscale") || equalsIgnoreCase(svar, _c("plot", "logscale"))) {param.x_log = true; param.y_log = true; i_prev = 19; break;}
			else if(equalsIgnoreCase(svar, "variable") || equalsIgnoreCase(svar, "var") || equalsIgnoreCase(svar, _c("plot", "variable"))) i_prev = 6;
			else if(equalsIgnoreCase(svar, "complex") || equalsIgnoreCase(svar, _c("plot", "complex"))) {i_prev = 20; eo2.allow_complex = true;}
			else if(equalsIgnoreCase(svar, "real") || equalsIgnoreCase(svar, _c("plot", "real"))) {i_prev = 21; eo2.allow_complex = false;}
			else if(i_prev == 15 || i_prev == 16 || i_prev == 17) {
				svalue = svar;
				svar = "";
				break;
			} else {
				size_t i2 = svar.rfind(SPACE);
				if(i2 == string::npos) {
					svalue.insert(0, svar);
					svar = "";
					break;
				}
				svalue.insert(0, svar.substr(i2));
				svar = svar.substr(0, i2);
				remove_blank_ends(svar);
				b_option = false;
			}
		}
		remove_blank_ends(svalue);
		if(!svalue.empty()) {
			if(i_prev == 15) {
				param.title = svalue;
			} else if(i_prev == 16) {
				param.y_label = svalue;
			} else if(i_prev == 17) {
				param.x_label = svalue;
			} else if(i_prev == 6) {
				if(svalue == "0") mvar = CALCULATOR->getVariableById(VARIABLE_ID_X);
				else if(svalue == "1") mvar = CALCULATOR->getVariableById(VARIABLE_ID_Y);
				else if(svalue == "2") mvar = CALCULATOR->getVariableById(VARIABLE_ID_Z);
				else {
					if(svalue[0] == '\\' && unicode_length(svalue) == 2) {
						mvar.set(svalue.substr(1), false, true);
					} else if(vargs[0].contains(MathStructure(svalue, true))) {
						mvar.set(svalue, false, true);
					} else {
						Variable *v = CALCULATOR->getActiveVariable(svalue);
						if(v && !v->isKnown()) mvar = v;
						else mvar.set(svalue, false, true);
					}
				}
			} else {
				if(i_prev < 0 || (i_prev != 2 && i_prev != 6)) {
					bool b_value = true;
					if(i_prev < 0 && i == 5 && old_step == 2 && svar.empty() && (svalue == "1" || svalue == "0")) {
						if(svalue == "1" && mstep.isZero()) mstep = i_rate;
						else if(svalue == "0" && !mstep.isZero() && mstep.isInteger()) {i_rate = mstep.number().intValue(); mstep.clear();}
					} else if(svar.empty() && equalsIgnoreCase(svalue, "x")) {mvar = CALCULATOR->getVariableById(VARIABLE_ID_X); if(old_step == 1 && i == 4) {old_step = 2;};}
					else if(svar.empty() && equalsIgnoreCase(svalue, "y")) {mvar = CALCULATOR->getVariableById(VARIABLE_ID_Y);if(old_step == 1 && i == 4) {old_step = 2;};}
					else if(svar.empty() && equalsIgnoreCase(svalue, "z")) {mvar = CALCULATOR->getVariableById(VARIABLE_ID_Z); if(old_step == 1 && i == 4) {old_step = 2;};}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "lines") || equalsIgnoreCase(svalue, _c("plot style", "lines")))) {style = PLOT_STYLE_LINES;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "points") || equalsIgnoreCase(svalue, _c("plot style", "points")))) {style = PLOT_STYLE_POINTS;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "dots") || equalsIgnoreCase(svalue, _c("plot style", "dots")))) {style = PLOT_STYLE_DOTS;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "boxes") || equalsIgnoreCase(svalue, "bars") || equalsIgnoreCase(svalue, _c("plot style", "boxes")))) {style = PLOT_STYLE_BOXES;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "histogram") || equalsIgnoreCase(svalue, "histeps") || equalsIgnoreCase(svalue, _c("plot style", "histogram")))) {style = PLOT_STYLE_HISTOGRAM;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "candlesticks") || equalsIgnoreCase(svalue, _c("plot style", "candlesticks")))) {style = PLOT_STYLE_CANDLESTICKS;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "steps") || equalsIgnoreCase(svalue, _c("plot style", "steps")))) {style = PLOT_STYLE_STEPS;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "linespoints") || equalsIgnoreCase(svalue, _c("plot style", "linespoints")))) {style = PLOT_STYLE_POINTS_LINES;}
					else if((i_prev == 4 || svar.empty()) && (equalsIgnoreCase(svalue, "polar") || equalsIgnoreCase(svalue, _c("plot", "polar")))) {style = PLOT_STYLE_POLAR;}
					else if((i_prev == 3 || svar.empty()) && (equalsIgnoreCase(svalue, "none") || equalsIgnoreCase(svalue, _c("plot smoothing", "none")))) {smoothing = PLOT_SMOOTHING_CSPLINES;}
					else if((i_prev == 3 || svar.empty()) && (equalsIgnoreCase(svalue, "splines") || equalsIgnoreCase(svalue, "csplines") || equalsIgnoreCase(svalue, _c("plot smoothing", "splines")))) {smoothing = PLOT_SMOOTHING_CSPLINES;}
					else if((i_prev == 3 || svar.empty()) && (equalsIgnoreCase(svalue, "bezier") || equalsIgnoreCase(svalue, _c("plot smoothing", "bezier")))) {smoothing = PLOT_SMOOTHING_BEZIER;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "none") || equalsIgnoreCase(svalue, _c("plot legend", "none")))) {param.legend_placement = PLOT_LEGEND_NONE;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "top-left") || equalsIgnoreCase(svalue, "top left") || equalsIgnoreCase(svalue, "topleft") || equalsIgnoreCase(svalue, _c("plot legend", "top-left")))) {param.legend_placement = PLOT_LEGEND_TOP_LEFT;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "top-right") || equalsIgnoreCase(svalue, "top right") || equalsIgnoreCase(svalue, "topright") || equalsIgnoreCase(svalue, _c("plot legend", "top-right")))) {param.legend_placement = PLOT_LEGEND_TOP_RIGHT;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "bottom-left") || equalsIgnoreCase(svalue, "bottom left") || equalsIgnoreCase(svalue, "bottomleft") || equalsIgnoreCase(svalue, _c("plot legend", "bottom-left")))) {param.legend_placement = PLOT_LEGEND_BOTTOM_LEFT;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "bottom-right") || equalsIgnoreCase(svalue, "bottm right") || equalsIgnoreCase(svalue, "bottomright") || equalsIgnoreCase(svalue, _c("plot legend", "bottom-right")))) {param.legend_placement = PLOT_LEGEND_BOTTOM_RIGHT;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "below") || equalsIgnoreCase(svalue, _c("plot legend", "below")))) {param.legend_placement = PLOT_LEGEND_BELOW;}
					else if((i_prev == 18 || svar.empty()) && (equalsIgnoreCase(svalue, "outside") || equalsIgnoreCase(svalue, _c("plot legend", "outside")))) {param.legend_placement = PLOT_LEGEND_OUTSIDE;}
					else b_value = false;
					if(b_value) {
						i_prev = -1;
						continue;
					}
				}
				if(i_prev >= 0) {
					MathStructure m;
					CALCULATOR->beginTemporaryStopMessages();
					CALCULATOR->beginTemporaryStopIntervalArithmetic();
					CALCULATOR->parse(&m, svalue, eo.parse_options);
					m.eval(eo);
					CALCULATOR->endTemporaryStopIntervalArithmetic();
					CALCULATOR->endTemporaryStopMessages();
					if((i_prev == 7 || i_prev == 8) && m.isNumber() && m.number().isReal()) {
						if(i_prev == 7) {param.y_min = m.number().floatValue(); param.auto_y_min = false;}
						else if(i_prev == 8) {param.y_max = m.number().floatValue(); param.auto_y_max = false;}
					} else if(m.isNumber() && m.number().isInteger()) {
						int i_value = m.number().intValue();
						switch(i_prev) {
							case 0: {
								if(i_value <= 10) mstep = m;
								else i_rate = i_value;
								if(vargs.size() == 6) old_step = 1;
								break;
							}
							case 1: {i_rate = i_value; mstep.clear(); break;}
							case 2: {mstep = m; break;}
							case 3: {
								if(i_value == 0) smoothing = PLOT_SMOOTHING_NONE;
								else if(i_value == 1) smoothing = PLOT_SMOOTHING_CSPLINES;
								else if(i_value == 2) smoothing = PLOT_SMOOTHING_BEZIER;
								else CALCULATOR->error(false, _("Illegal value: %s."), svalue.c_str(), NULL);
								break;
							}
							case 4: {
								if(i_value >= 0 && i_value <= PLOT_STYLE_DOTS) style = (PlotStyle) i_value;
								else CALCULATOR->error(false, _("Illegal value: %s."), svalue.c_str(), NULL);
								break;
							}
							case 5: {b_persistent = i_value; break;}
							case 9: {param.y_log_base = i_value; break;}
							case 10: {param.x_log_base = i_value; break;}
							case 19: {param.x_log_base = i_value; param.y_log_base = i_value; break;}
							case 11: {param.color = i_value; break;}
							case 12: {param.color = !i_value; break;}
							case 13: {param.grid = i_value; break;}
							case 14: {
								if(i_value > 0 && i_value < 100) param.linewidth = i_value;
								else CALCULATOR->error(false, _("Illegal value: %s."), svalue.c_str(), NULL);
								break;
							}
							case 18: {
								if(i_value >= 0 && i_value < PLOT_LEGEND_OUTSIDE) param.legend_placement = (PlotLegendPlacement) i_value;
								else CALCULATOR->error(false, _("Illegal value: %s."), svalue.c_str(), NULL);
								break;
							}
							case 20: {eo2.allow_complex = i_value; break;}
							case 21: {eo2.allow_complex = !i_value; break;}
						}
					} else if((i_prev == 0 && (m.isNumber() || svalue.find_first_of(NUMBERS) != string::npos)) || i_prev == 2) {
						mstep = m;
					} else {
						CALCULATOR->error(false, _("Illegal value: %s."), svalue.c_str(), NULL);
					}
				} else {
					CALCULATOR->error(false, _("Unrecognized option: %s."), svalue.c_str(), NULL);
				}
			}
			i_prev = -1;
		}
	}
	mstruct = vargs[0];
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	if((!mstruct.contains(mvar, true) && !contains_rand(mstruct)) || (!mstruct.isVector() && (!mstruct.isFunction() || (mstruct.function()->id() != FUNCTION_ID_HORZCAT && mstruct.function()->id() != FUNCTION_ID_VERTCAT)) && !mstruct.representsScalar())) {
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_HORZCAT || mstruct.function()->id() == FUNCTION_ID_VERTCAT)) mstruct.setType(STRUCT_VECTOR);
		if(!mstruct.isVector() && (vargs[0].contains(mvar, true) || contains_rand(vargs[0]))) mstruct = vargs[0];
	} else {
		eo2.calculate_functions = false;
		eo2.expand = false;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		int im = 0;
		if(CALCULATOR->endTemporaryStopMessages(NULL, &im) > 0 || im > 0) mstruct = vargs[0];
		eo2.calculate_functions = eo.calculate_functions;
		eo2.expand = eo.expand;
	}
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_HORZCAT || mstruct.function()->id() == FUNCTION_ID_VERTCAT)) mstruct.setType(STRUCT_VECTOR);
	if(mstruct.isMatrix() && mstruct.columns() == 1 && mstruct.rows() > 1) {
		if(!mstruct.transposeMatrix()) return 0;
	}
	CALCULATOR->endTemporaryStopIntervalArithmetic();
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
		dpd->style = style;
		dpd->smoothing = smoothing;
		dpds.push_back(dpd);
	} else if(mstruct.isVector()) {
		int matrix_index = 1, vector_index = 1;
		if(mstruct.size() > 0 && (mstruct[0].isVector() || mstruct[0].contains(mvar, false, true, true))) {
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
					dpd->style = style;
					dpd->smoothing = smoothing;
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
					dpd->style = style;
					dpd->smoothing = smoothing;
					vector_index++;
					dpds.push_back(dpd);
				} else {
					MathStructure y_vector;
					if(!mstep.isZero()) {
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->beginTemporaryStopIntervalArithmetic();
						generate_plotvector(mstruct[i], mvar, vargs[1], vargs[2], mstep, x_vector, y_vector, eo2);
						CALCULATOR->endTemporaryStopIntervalArithmetic();
						CALCULATOR->endTemporaryStopMessages();
						if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
					} else if(i_rate < 1) {
						CALCULATOR->error(true, _("Sampling rate must be a positive integer."), NULL);
					} else {
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->beginTemporaryStopIntervalArithmetic();
						if(i_rate <= 1000000) generate_plotvector(mstruct[i], mvar, vargs[1], vargs[2], i_rate, x_vector, y_vector, eo2);
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
						MathStructure mprint;
						if(vargs[0].isVector() && vargs[0].size() == mstruct.size()) mprint = vargs[0][i];
						else mprint = mstruct[i];
						dpd->title = format_and_print(mprint);
						dpd->test_continuous = true;
						dpd->style = style;
						dpd->smoothing = smoothing;
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
			dpd->style = style;
			dpd->smoothing = smoothing;
			dpds.push_back(dpd);
		}
	} else {
		MathStructure x_vector, y_vector;
		if(!mstep.isZero()) {
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->beginTemporaryStopIntervalArithmetic();
			generate_plotvector(mstruct, mvar, vargs[1], vargs[2], mstep, x_vector, y_vector, eo2);
			CALCULATOR->endTemporaryStopIntervalArithmetic();
			CALCULATOR->endTemporaryStopMessages();
			if(y_vector.size() == 0) CALCULATOR->error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
		} else if(i_rate < 1) {
			CALCULATOR->error(true, _("Sampling rate must be a positive integer."), NULL);
		} else {
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->beginTemporaryStopIntervalArithmetic();
			if(i_rate <= 1000000) generate_plotvector(mstruct, mvar, vargs[1], vargs[2], i_rate, x_vector, y_vector, eo2);
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
			MathStructure mprint(vargs[0]);
			dpd->title = format_and_print(mprint);
			dpd->test_continuous = true;
			dpd->style = style;
			dpd->smoothing = smoothing;
			dpds.push_back(dpd);
		}
	}
	if(x_vectors.size() > 0 && !CALCULATOR->aborted()) {
		CALCULATOR->plotVectors(&param, y_vectors, x_vectors, dpds, b_persistent, 0);
		for(size_t i = 0; i < dpds.size(); i++) {
			if(dpds[i]) delete dpds[i];
		}
	}
	mstruct.clear();
	return 1;

}

GeographicDistanceFunction::GeographicDistanceFunction() : MathFunction("geodistance", 4, 4) {
}
int GeographicDistanceFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure m(vargs);
	m[0].eval(eo); m[1].eval(eo); m[2].eval(eo); m[3].eval(eo);
	Number nr_pi; nr_pi.pi();
	Number ln1, la1, ln2, la2;
	bool reverse = false;
	for(size_t i = 0; i < 4; i++) {
		if(i == 2) reverse = false;
		if(m[i].isSymbolic()) {
			string s = m[i].symbol();
			remove_blank_ends(s);
			bool neg = false;
			if(!s.empty()) {
				if((i == 0 || i == 2) && (s[s.length() - 1] == 'W' || s[s.length() - 1] == 'E')) {
					reverse = true;
				}
				if(s[s.length() - 1] == 'N' || s[s.length() - 1] == 'E') {
					s.erase(s.length() - 1, 1);
				} else if(s[s.length() - 1] == 'S' || s[s.length() - 1] == 'W') {
					s.erase(s.length() - 1, 1);
					neg = true;
				}
			}
			CALCULATOR->parse(&m[i], s, eo.parse_options);
			if(neg) m[i].negate();
			m[i].eval(eo);
		}
		if(m[i].isNumber()) {
			m[i].number() *= nr_pi; m[i].number() /= 180;
		} else {
			m[i] /= CALCULATOR->getRadUnit();
			m[i].eval(eo);
			if(!m[i].isNumber()) return 0;
		}
		if(!m[i].number().isReal()) return 0;
		int prec = m[i].number().precision(true);
		if(prec < 0 || prec >= 15) m[i].number().intervalToMidValue();
		if(reverse) {
			if(i == 0) ln1 = m[i].number();
			if(i == 1) la1 = m[i].number();
			if(i == 2) ln2 = m[i].number();
			if(i == 3) la2 = m[i].number();
		} else {
			if(i == 0) la1 = m[i].number();
			if(i == 1) ln1 = m[i].number();
			if(i == 2) la2 = m[i].number();
			if(i == 3) ln2 = m[i].number();
		}
	}
	if(la1 > nr_pi || ln1 > nr_pi || la2 > nr_pi || ln2 > nr_pi) return 0;
	nr_pi.negate();
	if(la1 < nr_pi || ln1 < nr_pi || la2 < nr_pi || ln2 < nr_pi) return 0;
	nr_pi.negate();
	if(la1.isInterval()) {
		mstruct = la1.lowerEndPoint(); mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		MathStructure m2(la1.upperEndPoint()); m2 *= CALCULATOR->getRadUnit();
		m2.transform(this);
		m2.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		mstruct.addChild(m2);
		mstruct.addChild(m_zero);
		return 1;
	}
	if(ln1.isInterval()) {
		mstruct = la1; mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.addChild(ln1.lowerEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		MathStructure m2(la1); m2 *= CALCULATOR->getRadUnit();
		m2.transform(this);
		m2.addChild(ln1.upperEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		mstruct.addChild(m2);
		mstruct.addChild(m_zero);
		return 1;
	}
	if(la2.isInterval()) {
		mstruct = la1; mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(la2.lowerEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		MathStructure m2(la1); m2 *= CALCULATOR->getRadUnit();
		m2.transform(this);
		m2.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(la2.upperEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(ln2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		mstruct.addChild(m2);
		mstruct.addChild(m_zero);
		return 1;
	}
	if(ln2.isInterval()) {
		mstruct = la1; mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.addChild(ln2.lowerEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		MathStructure m2(la1); m2 *= CALCULATOR->getRadUnit();
		m2.transform(this);
		m2.addChild(ln1); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(la2); mstruct.last() *= CALCULATOR->getRadUnit();
		m2.addChild(ln2.upperEndPoint()); mstruct.last() *= CALCULATOR->getRadUnit();
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		mstruct.addChild(m2);
		mstruct.addChild(m_zero);
		return 1;
	}
	if(ln1 == ln2 && la1 == la2) {
		mstruct.clear();
		Unit *unit_meter = CALCULATOR->getUnit("m");
		if(unit_meter) mstruct *= unit_meter;
		return 1;
	}
	bool b_failed = false;
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	Number a(6378137L), b("6356752.314245"), f("298.257223563");
	f.recip();
	Number p1mf(1, 1, 0); p1mf -= f;
	Number tanU1(la1);
	Number tanU2(la2);
	if(b_failed || !tanU1.tan() || !tanU1.multiply(p1mf) || !tanU2.tan() || !tanU2.multiply(p1mf)) b_failed = true;
	Number cosU1(tanU1);
	Number cosU2(tanU2);
	if(b_failed || !cosU1.square() || !cosU1.add(1) || !cosU1.sqrt() || !cosU1.recip() || !cosU2.square() || !cosU2.add(1) || !cosU2.sqrt() || !cosU2.recip()) b_failed = true;
	Number sinU1(tanU1);
	Number sinU2(tanU2);
	if(!sinU1.multiply(cosU1) || !sinU2.multiply(cosU2)) b_failed = true;
	Number sinU1U2(sinU1);
	if(b_failed || !sinU1U2.multiply(sinU2)) b_failed = true;
	Number L(ln1); L -= ln2;
	Number lambda(L);
	Number tmp, sin_sigma, cos_sigma(1, 1, 0), sigma, sin_a, cos2a(1, 1, 0), cos2sm(1, 1, 0), C, tmp2;
	bool antipodal = false;
	tmp = L; tmp.abs();
	if(tmp > nr_pi / 2) antipodal = true;
	if(!antipodal) {
		tmp = la1; tmp -= la2; tmp.abs();
		if(tmp < nr_pi / 2) antipodal = true;
	}
	if(antipodal) {cos_sigma.set(-1 ,1 ,0); sigma = nr_pi;}
	Number prec(1, 1, -12);
	for(size_t i = 0; !b_failed; i++) {
		if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		sin_sigma = lambda;
		tmp = cosU1;
		if(!sin_sigma.cos() || !sin_sigma.multiply(cosU2) || !sin_sigma.multiply(sinU1) || !tmp.multiply(sinU2) || !sin_sigma.negate() || !sin_sigma.add(tmp) || !sin_sigma.square()) {b_failed = true; break;}
		tmp = lambda;
		if(!tmp.sin() || !tmp.multiply(cosU2) || !tmp.square() || !sin_sigma.add(tmp)) {b_failed = true; break;}
		tmp = sin_sigma;
		if(!tmp.abs()) {b_failed = true; break;}
		tmp = tmp2; tmp2++;
		if(tmp.isOne()) break;
		if(!sin_sigma.sqrt()) {b_failed = true; break;}
		cos_sigma = lambda;
		if(!cos_sigma.cos() || !cos_sigma.multiply(cosU1) || !cos_sigma.multiply(cosU2) || !cos_sigma.add(sinU1U2)) {b_failed = true; break;}
		sigma = sin_sigma;
		if(!sigma.atan2(cos_sigma)) {b_failed = true; break;}
		sin_a = lambda;
		if(!sin_a.sin() || !sin_a.divide(sin_sigma) || !sin_a.multiply(cosU2) || !sin_a.multiply(cosU1)) {b_failed = true; break;}
		cos2a = sin_a;
		if(!cos2a.square() || !cos2a.negate() || !cos2a.add(1)) {b_failed = true; break;}
		if(cos2a.isNonZero()) {
			cos2sm = sinU1U2;
			if(!cos2sm.divide(cos2a) || !cos2sm.multiply(-2) || !cos2sm.add(cos_sigma)) {b_failed = true; break;}
		} else {
			cos2sm.clear();
		}
		C = cos2a;
		if(!C.multiply(-3) || !C.add(4) || !C.multiply(f) || !C.add(4) || !C.multiply(cos2a) || !C.multiply(f) || !C.divide(16)) {b_failed = true; break;}
		tmp = lambda;
		lambda = cos2sm;
		if(!lambda.square() || !lambda.multiply(2) || !lambda.add(-1) || !lambda.multiply(cos_sigma) || !lambda.multiply(C) || !lambda.add(cos2sm) || !lambda.multiply(sin_sigma) || !lambda.multiply(C) || !lambda.add(sigma) || !lambda.multiply(sin_a) || !lambda.multiply(f) || !C.negate() || !C.add(1) || !lambda.multiply(C) || !lambda.add(L)) {b_failed = true; break;}
		tmp2 = lambda;
		if(!tmp2.abs()) {b_failed = true; break;}
		if(antipodal && !tmp2.subtract(nr_pi)) {b_failed = true; break;}
		if(tmp2 > nr_pi) {b_failed = true; break;}
		tmp -= lambda; tmp.abs();
		if(tmp < prec) break;
		if(i > 200) {b_failed = true; break;}
	}
	if(!b_failed) {
		Number b2(b);
		Number u2(a);
		if(!u2.square() || !b2.square() || !u2.subtract(b2) || !u2.divide(b2) || !u2.multiply(cos2a)) return 0;
		Number A(u2);
		if(!A.multiply(-175) || !A.add(320) || !A.multiply(u2) || !A.add(-768) || !A.multiply(u2) || !A.add(4096) || !A.multiply(u2) || !A.divide(16384) || !A.add(1)) b_failed = true;
		Number B(u2);
		if(b_failed || !B.multiply(-47) || !B.add(74) || !B.multiply(u2) || !B.add(-128) || !B.multiply(u2) || !B.add(256) || !B.multiply(u2) || !B.divide(1024)) b_failed = true;
		Number cos2sm2(cos2sm);
		if(b_failed || !cos2sm2.square()) b_failed = true;
		Number sigma_delta(cos2sm2);
		tmp = sin_sigma;
		if(b_failed || !sigma_delta.multiply(4) || !sigma_delta.add(-3) || !tmp.square() || !tmp.multiply(4) || !tmp.add(-3) || !sigma_delta.multiply(tmp) || !sigma_delta.multiply(cos2sm) || !sigma_delta.multiply(B) || !sigma_delta.divide(-6) || !cos2sm2.multiply(2) || !cos2sm2.add(-1) || !cos2sm2.multiply(cos_sigma) || !sigma_delta.add(cos2sm2) || !sigma_delta.multiply(B) || !sigma_delta.divide(4) || !sigma_delta.add(cos2sm) || !sigma_delta.multiply(sin_sigma) || !sigma_delta.multiply(B)) b_failed = true;
		Number s(sigma);
		if(b_failed || !s.subtract(sigma_delta) || !s.multiply(A) || !s.multiply(b)) b_failed = true;
		if(!b_failed) {
			if(s > 1000000L) s.setUncertainty(nr_half);
			else if(s < 1000) s.setUncertainty(Number(1, 2, -3));
			else s.setRelativeUncertainty(Number(1, 2, -6));
			mstruct = s;
			Unit *unit_meter = CALCULATOR->getUnit("m");
			if(unit_meter) mstruct *= unit_meter;
		}
	}
	if(b_failed) {
		Number a(ln2);
		if(!a.subtract(ln1) || !a.divide(2) || !a.sin() || !a.square()) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		tmp = la1; if(!tmp.cos() || !a.multiply(tmp)) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		tmp = la2; if(!tmp.cos() || !a.multiply(tmp)) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		tmp = la2; if(!tmp.subtract(la1) || !tmp.divide(2) || !tmp.sin() || !tmp.square() || !a.add(tmp)) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		Number d(a);
		if(!d.sqrt() || !a.negate() || !a.add(1) || !a.sqrt() || !d.atan2(a) || !d.multiply(2) || !d.multiply(6371) || !d.multiply(1000)) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
		d.setRelativeUncertainty(Number(6, 1, -3));
		mstruct = d;
		Unit *unit_meter = CALCULATOR->getUnit("m");
		if(unit_meter) mstruct *= unit_meter;
	}
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	return 1;
}

void get_latex_args(const string &str, size_t &i, string *s1 = NULL, string *s2 = NULL, string *opt = NULL, bool supsub = false) {
	int in_sqb = 0, in_cub = 0;
	size_t index = 0, cub_index = 0;
	for(; i < str.length(); i++) {
		if(index == 0 && str[i] == '\\' && str.find("limits", i + 1) == i + 1) {
			i += 6;
		} else if(index == 0 && str[i] == '\\' && str.find("nolimits", i + 1) == i + 1) {
			i += 8;
		} else if(index == 0 && str[i] == '[') {
			if(!in_sqb) cub_index = i;
			in_sqb++;
		} else if(index == 0 && in_sqb && str[i] == ']') {
			in_sqb--;
			if(in_sqb == 0 && opt) *opt = str.substr(cub_index + 1, i - (cub_index + 1));
		} else if(supsub && !in_cub && !in_sqb && ((str[i] == '_' && s1) || (str[i] == '^' && s2))) {
			if(str[i] == '_') index = 1;
			else index = 2;
			size_t i2 = str.find_first_not_of(SPACES, i + 1);
			if(i2 != string::npos) {
				if(str[i2] == '{') {
					in_cub++;
					cub_index = i2;
					i = i2;
				} else if(str[i2] == '\\') {
					cub_index = i2;
					i2++;
					for(; i2 < str.length(); i2++) {
						if((str[i2] < 'a' || str[i2] > 'z') && (str[i2] < 'A' || str[i2] > 'Z')) {
							break;
						}
					}
					if(i2 == cub_index + 1) i2++;
					if(index == 1) *s1 = str.substr(cub_index, i2 - cub_index);
					else if(index == 2) *s2 = str.substr(cub_index, i2 - cub_index);
					i = i2 - 1;
				} else {
					size_t l = 1;
					if((signed char) str[i2] < 0) {
						while(i2 + l < str.length() && (signed char) str[i2 + l] < 0 && (unsigned char) str[i2 + l] < 0xC0) l++;
					}
					if(index == 1) *s1 = str.substr(i2, l);
					else if(index == 2) *s2 = str.substr(i2, l);
					i = i2 + l - 1;
				}
				if(!cub_index) {
					index = 0;
					if((!s1 || !s1->empty()) && (!s2 || !s2->empty())) {i++; break;}
				}
			}
		} else if(!in_sqb && (!supsub || in_cub) && str[i] == '{' && s1) {
			if(!in_cub) {
				cub_index = i;
				index++;
			}
			in_cub++;
		} else if(in_cub && str[i] == '}') {
			in_cub--;
			if(in_cub == 0) {
				if(index == 1) {
					*s1 = str.substr(cub_index + 1, i - (cub_index + 1));
					if(s1->empty()) *s1 += " ";
					if(!s2 || (supsub && !s2->empty())) {i++; break;}
				} else if(index == 2) {
					*s2 = str.substr(cub_index + 1, i - (cub_index + 1));
					if(s2->empty()) *s2 += " ";
					i++;
					if(!supsub || !s1 || !s1->empty()) break;
				}
			}
		} else if(!in_sqb && !in_cub && str[i] != ' ' && str[i] != '\t') {
			break;
		}
	}
	if(in_cub) {
		if(index == 1) {
			*s1 = str.substr(cub_index + 1);
			if(s1->empty()) *s1 += " ";
		} else if(index == 2) {
			*s2 = str.substr(cub_index + 1);
			if(s2->empty()) *s2 += " ";
		}
	}
}
void read_latex_num(string &str, bool cplx = false, bool angle = false) {
	if(cplx) gsub(":", "∠", str);
	if(angle) gsub(";", ":", str);
	gsub(",", ".", str);
	gsub("e", "E", str);
	gsub("d", "E", str);
	gsub("D", "E", str);
	gsub("\\pm", "+/-", str);
	gsub("+-", "+/-", str);
	gsub("x", "*", str);
}

void parse_latex_string(string &str, bool in_unit = false, bool symbols_only = false, bool preserve_i = false) {
	bool square = false, cubic = false;
	for(size_t i = 0; i < str.size();) {
		if(str[i] == '$' || str[i] == '{' || str[i] == '}' || str[i] == '&') {
			str.erase(i, 1);
		} else if(str[i] == '~') {
			str[i] = ' ';
			i++;
		} else if(str[i] == '[') {
			str[i] = '(';
			i++;
		} else if(str[i] == ']') {
			str[i] = ')';
			i++;
		} else if(str[i] == 'i' && !preserve_i && (i == str.length() - 1 || ((str[i + 1] < 'a' || str[i + 1] > 'w') && (str[i + 1] < 'A' || str[i + 1] > 'Z')))) {
			str.insert(i, 1, '\\');
			i += 2;
		} else if(in_unit && str[i] == '.') {
			str[i] = '*';
			i++;
		} else if(str[i] == '\\') {
			size_t i2 = i + 1;
			for(; i2 < str.length(); i2++) {
				if((str[i2] < 'a' || str[i2] > 'z') && (str[i2] < 'A' || str[i2] > 'Z')) {
					break;
				}
			}
			if(i2 == i + 1) i2++;
			string s = str.substr(i + 1, i2 - (i + 1));
			string snew;
			bool unit_macro = in_unit;
			if(in_unit) {
				bool nonunit = false;
				if(s == "square") {square = true; nonunit = true;}
				else if(s == "cubic") {cubic = true; nonunit = true;}
				else if(s == "squared") {
					if(i > 0 && str[i - 1] == ' ') i--;
					snew = "^(2)";
					nonunit = true;
				} else if(s == "cubed") {
					if(i > 0 && str[i - 1] == ' ') i--;
					snew = "^(3)";
					nonunit = true;
				} else if(s == "raiseto" || s == "tothe") {snew = "^"; nonunit = true;}
				else if(s == "highlight" || s == "of") {
					string s1;
					get_latex_args(str, i2, &s1);
					nonunit = true;
				} else if(s == "per") {snew = "/"; nonunit = true;}
				else if(s == "ampere") snew = "A";
				else if(s == "candela") snew = "cd";
				else if(s == "kelvin") snew = "K";
				else if(s == "kilogram") snew = "kg";
				else if(s == "gram") snew = "g";
				else if(s == "metre" || s == "meter") snew = "m";
				else if(s == "mole") snew = "mol";
				else if(s == "second") snew = "s";
				else if(s == "becquerel") snew = "Bq";
				else if(s == "degreeCelsius") snew = "oC";
				else if(s == "coulomb") snew = "C";
				else if(s == "farad") snew = "F";
				else if(s == "gray") snew = "Gy";
				else if(s == "hertz") snew = "Hex";
				else if(s == "henry") snew = "H";
				else if(s == "joule") snew = "J";
				else if(s == "lumen") snew = "lm";
				else if(s == "katal") snew = "kat";
				else if(s == "lux") snew = "lx";
				else if(s == "newton") snew = "N";
				else if(s == "ohm") snew = "ohm";
				else if(s == "pascal") snew = "Pa";
				else if(s == "radian") snew = "rad";
				else if(s == "seiemens") snew = "S";
				else if(s == "sievert") snew = "Sv";
				else if(s == "steradian") snew = "sr";
				else if(s == "tesla") snew = "T";
				else if(s == "volt") snew = "V";
				else if(s == "watt") snew = "W";
				else if(s == "weber") snew = "Wb";
				else if(s == "astronomicalunit") snew = "au";
				else if(s == "bel") snew = "B";
				else if(s == "dalton") snew = "Da";
				else if(s == "day") snew = "d";
				else if(s == "decibel") snew = "dB";
				else if(s == "degree") snew = "deg";
				else if(s == "electronvolt") snew = "eV";
				else if(s == "hectare") snew = "ha";
				else if(s == "hour") snew = "h";
				else if(s == "litre") snew = "L";
				else if(s == "liter") snew = "L";
				else if(s == "arcminute") snew = "arcmin";
				else if(s == "minute") snew = "min";
				else if(s == "arcsecond") snew = "arcsec";
				else if(s == "neper") snew = "Np";
				else if(s == "tonne") snew = "t";
				else if(s == "atomicmassunit") snew = "u";
				else if(s == "angstrom") snew = "Å";
				else if(s == "bar") snew = "bar";
				else if(s == "barn") snew = "b";
				else if(s == "knot") snew = "knot";
				else if(s == "mmHg") snew = "mmHg";
				else if(s == "nauticalmile") snew = "nautical_mile";
				else if(s == "bohr") snew = "bohr_unit";
				else if(s == "clight") snew = "c_unit";
				else if(s == "electronmass") snew = "electron_unit";
				else if(s == "elementarycharge") snew = "e_unit";
				else if(s == "hartree") snew = "Ha";
				else if(s == "planckbar") snew = "planck_unit";
				else if(s == "kWh") snew = "kWh";
				else if(CALCULATOR->getPrefix(s)) {snew = s; nonunit = true;}
				else if(s == "deka") {snew = "deca"; nonunit = true;}
				else {
					ParseOptions po;
					MathStructure mtest;
					CALCULATOR->beginTemporaryStopMessages();
					CALCULATOR->parse(&mtest, s, po);
					CALCULATOR->endTemporaryStopMessages();
					if(mtest.isUnit()) {
						snew = s;
					} else {
						unit_macro = false;
						nonunit = true;
					}
				}
				if(!nonunit) {
					if(square) {
						if(i > 0 && str[i - 1] == ' ') i--;
						snew += "^(2)";
						square = false;
						cubic = false;
					} else if(cubic) {
						if(i > 0 && str[i - 1] == ' ') i--;
						snew += "^(3)";
						cubic = false;
					} else {
						snew += " ";
					}
				}
			}
			if(!unit_macro) {
				if(s == "Gamma") {snew = "Γ";}
				else if(s == "Delta") {snew = "Δ";}
				else if(s == "Lambda") {snew = "Λ";}
				else if(s == "Phi") {snew = "Φ";}
				else if(s == "Pi") {snew = "Π";}
				else if(s == "Psi") {snew = "Ψ";}
				else if(s == "Sigma") {snew = "Σ";}
				else if(s == "Theta") {snew = "Θ";}
				else if(s == "Upsilon") {snew = "Υ";}
				else if(s == "Xi") {snew = "Ξ";}
				else if(s == "Omega") {snew = "Ω";}
				else if(s == "alpha") {snew = "α";}
				else if(s == "beta") {snew = "β";}
				else if(s == "gamma") {snew = "γ";}
				else if(s == "delta") {snew = "δ";}
				else if(s == "epsilon") {snew = "ϵ";}
				else if(s == "zeta") {snew = "ζ";}
				else if(s == "eta") {snew = "η";}
				else if(s == "theta") {snew = "θ";}
				else if(s == "iota") {snew = "ι";}
				else if(s == "kappa") {snew = "κ";}
				else if(s == "lampda") {snew = "λ";}
				else if(s == "mu" || s == "textmu" || s == "micro") {snew = "μ";}
				else if(s == "nu") {snew = "ν";}
				else if(s == "xi") {snew = "ξ";}
				else if(s == "pi") {snew = "π";}
				else if(s == "rho") {snew = "ρ";}
				else if(s == "sigma") {snew = "σ";}
				else if(s == "tau") {snew = "τ";}
				else if(s == "upsilon") {snew = "υ";}
				else if(s == "phi") {snew = "ϕ";}
				else if(s == "chi") {snew = "χ";}
				else if(s == "psi") {snew = "ψ";}
				else if(s == "omega") {snew = "ω";}
				else if(s == "digamma") {snew = "ϝ";}
				else if(s == "varepsilon") {snew = "ε";}
				else if(s == "varkappa") {snew = "ϰ";}
				else if(s == "varphi") {snew = "φ";}
				else if(s == "varpi") {snew = "ϖ";}
				else if(s == "varrho") {snew = "ϱ";}
				else if(s == "varsigma") {snew = "ς";}
				else if(s == "vartheta") {snew = "ϑ";}
				else if(s == "varGamma") {snew = "Γ";}
				else if(s == "varDelta") {snew = "Δ";}
				else if(s == "varLambda") {snew = "Λ";}
				else if(s == "varPhi") {snew = "Φ";}
				else if(s == "varPi") {snew = "Π";}
				else if(s == "varPsi") {snew = "Ψ";}
				else if(s == "varSigma") {snew = "Σ";}
				else if(s == "varTheta") {snew = "Θ";}
				else if(s == "varUpsilon") {snew = "Υ";}
				else if(s == "varXi") {snew = "Ξ";}
				else if(s == "varOmega") {snew = "Ω";}
				else if(s == "infty") {snew = "∞";}
				else if(s == "hbar") {snew = "ℏ";}
				else if(s == "haslash") {snew = "ℏ";}
				else if(s == "eth") {snew = "ð";}
				else if(s == "mho") {snew = "℧";}
				else if(s == "re") {snew = "ℜ";}
				else if(s == "im") {snew = "ℑ";}
				else if(s == "pm") {snew = "+/-";}
				else if(s == "AA") {snew = "Å";}
				else if(s == "AE") {snew = "Æ";}
				else if(s == "O") {snew = "Ø";}
				else if(s == "OE") {snew = "Œ";}
				else if(s == "aa") {snew = "å";}
				else if(s == "ae") {snew = "æ";}
				else if(s == "o") {snew = "ø";}
				else if(s == "oe") {snew = "œ";}
				else if(s == "celsius") {snew = "℃";}
				else if(s == "degree") {snew = "°";}
				else if(s == "perthousand" || s == "textperthousand") {snew = "‰";}
				else if(s == "&") {snew = "&";}
				else if(s == "{") {snew = "{";}
				else if(s == "}") {snew = "}";}
				else if(s == "%") {snew = "%";}
				else if(s == "\\" || s == "*" || s == "[" || s == "]") {snew = " ";}
				else if(s == "$" || s == "mathdollar" || s == "textdollar") {snew = "$";}
				else if(s == "matheuro" || s == "texteuro") {snew = "€";}
				else if(s == "mathsterling" || s == "textsterling" || s == "pounds") {snew = "£";}
				else if(s == "mathunderscore" || s == "textunderscore" || s == "_") {snew = "_";}
				else if(s == "times") {snew = "*";}
				else if(s == "div") {snew = "/";}
				else if(s == "colon") {snew = ":";}
				else if(s == "dots" || s == "cdots" || s == "ldots" || s == "dotsb" || s == "dotsi" || s == "dotsm" || s == "dotso" || s == "mathellipsis") {snew = "...";}
				else if(s == "subseteq" || s == "subseteqq") {snew = "⊆";}
				else if(s == "subsetneq" || s == "subsetneqq") {snew = "⊊";}
				else if(s == "supseteq" || s == "supseteqq") {snew = "⊇";}
				else if(s == "supsetneq" || s == "supsetneqq") {snew = "⊋";}
				else if(s == "in") {snew = "∈";}
				else if(s == "notin") {snew = "∉";}
				else if(s == "ni" || s == "own") {snew = "∋";}
				else if(s == "geq" || s == "geqq" || s == "geqslant") {snew = ">=";}
				else if(s == "leq" || s == "leqq" || s == "leqslant") {snew = "<=";}
				else if(s == "neq") {snew = "!=";}
				else if(s == "rvert" || s == "lvert" || s == "vert" || s == "textbar") {snew = "|";}
				else if(s == "|" || s == "lVert" || s == "rVert" || s == "Vert") {snew = "||";}
				else if(s == "lfloor") {snew = "⌊";}
				else if(s == "rfloor") {snew = "⌋";}
				else if(s == "lceil") {snew = "⌈";}
				else if(s == "rceil") {snew = "⌉";}
				else if(s == "backslash" || s == "textbackslash") {snew = "\\";}
				else if(s == "textless") {snew = "<";}
				else if(s == "textgreater") {snew = ">";}
				else if(s == "bmod" || s == "mod") {snew = "mod";}
				else if(s == "qquad" || s == "quad" || s == "nobreakspace" || s == "thinspace" || s == "medspace" || s == "thickspace" || s == " ") snew = " ";
				else if(s == "negthinspace" || s == "negmedspace" || s == "negthickspace") snew = "";
				else if(s.empty()) snew = " ";
				else if(s == "hspace" || s == "vspace" || s == "mspace" || s == "phantom" || s == "label" || s == "ref" || s == "displaybreak" || s == "pagebreak" || s == "intertext" || s == "shortintertext" || s == "tag") {
					string s1;
					get_latex_args(str, i2, &s1);
					if(s != "phantom" || !s1.empty()) snew = " ";
				} else if(!symbols_only) {
					if(s == "displaystyle" || s == "textstyle" || s == "scriptstyle" || s == "scriptscriptstyle" || s == "left" || s == "right" || s == "bigl" || s == "biggl" || s == "Bigl" || s == "Biggl" || s == "limits" || s == "nolimits" || s == "nobreakdash" || s == "notag") snew = "";
					else if(s == "text" || s == "mbox") {
						get_latex_args(str, i2, &snew);
						remove_blank_ends(snew);
						if(snew != "and" && snew != "or" && snew != "xor" && (i == 0 || str[i - 1] != '_') && (i2 >= str.length() || str[i2] != '_')) {
							snew.insert(0, "\"");
							snew += "\"";
						}
					} else if(s == "mathrm" || s == "mathbf" || s == "mathit" || s == "mathcal" || s == "mathbb" || s == "mathfrak" || s == "mathsf" || s == "mathtt" || s == "boxed" || s == "smash") {
						get_latex_args(str, i2, &snew);
						parse_latex_string(snew, false, false, true);
					} else if(s == "arccos" || s == "arcsin" || s == "arctan" || s == "arg" || s == "cos" || s == "cosh" || s == "cot" || s == "coth" || s == "csc" || s == "det" || s == "exp" || s == "gcd" || s == "lg" || s == "ln" || s == "log" || s == "max" || s == "min" || s == "sec" || s == "sin" || s == "sinh" || s == "tan" || s == "tanh") {
						snew = s; snew += " ";
					} else if(s == "deg" || s == "hom" || s == "inf" || s == "dim" || s == "inflim" || s == "ker" || s == "liminf" || s == "limsup" || s == "Pr" || s == "projlim" || s == "sup" || s == "varlimsup" || s == "varliminf" || s == "varprojlim" || s == "varinjlim") {
						CALCULATOR->error(true, "Unsupported LaTeX command/macro %s.", (string("\\") + s).c_str(), NULL);
					} else if(s == "frac" || s == "dfrac" || s == "tfrac" || s == "cfrac" || s == "sfrac" || s == "nicefrac") {
						string s1, s2;
						get_latex_args(str, i2, &s1, &s2);
						parse_latex_string(s1);
						parse_latex_string(s2);
						snew = "("; snew += s1; snew += ")/("; snew += s2; snew += ")";
					} else if(s == "lim") {
						string s1, s2;
						get_latex_args(str, i2, &s1, NULL, NULL, true);
						size_t i3 = s1.find("\\to");
						int sgn = 0;
						if(i3 != string::npos) {
							s2 = s1.substr(0, i3);
							s1 = s1.substr(i3 + 3);
						}
						remove_blank_ends(s1);
						if(!s1.empty() && s1[s1.length()] == '+') sgn = 1;
						else if(!s1.empty() && s1[s1.length()] == '-') sgn = -1;
						if(sgn != 0) s1.erase(s1.length() - 1, 1);
						parse_latex_string(s1);
						parse_latex_string(s2);
						snew = "limit(";
						string sarg = str.substr(i2);
						parse_latex_string(sarg);
						snew += sarg;
						snew += ",";
						snew += s1;
						if(!s2.empty() || sgn != 0) {
							snew += ",";
							snew += s2;
						}
						if(sgn != 0) {
							snew += ",";
							snew += i2s(sgn);
						}
						snew += ")";
						i2 = str.length();
					} else if(s == "binom" || s == "dbinom" || s == "tbinom") {
						string s1, s2;
						get_latex_args(str, i2, &s1, &s2);
						parse_latex_string(s1);
						parse_latex_string(s2);
						snew = "binomial("; snew += s1; snew += ","; snew += s2; snew += ")";
					} else if(s == "operatorname") {
						get_latex_args(str, i2, &snew);
						parse_latex_string(snew, false, true);
						MathFunction *f = CALCULATOR->getActiveFunction(snew);
						if(!f) {
							string snew2 = snew;
							gsub("_", "", snew2);
							f = CALCULATOR->getActiveFunction(snew);
							if(f) snew = snew2;
						}
					} else if(s == "sqrt") {
						string s1, opt;
						get_latex_args(str, i2, &s1, NULL, &opt);
						if(opt.empty()) snew = "sqrt";
						else snew = "root";
						parse_latex_string(s1);
						parse_latex_string(opt);
						if(s1.empty()) {
							if(opt.empty()) {
								snew += " ";
							} else {
							}
						} else {
							if(opt.empty()) {
								snew += "("; snew += s1; snew += ")";
							} else {
								snew += "("; snew += s1; snew += ","; snew += opt; snew += ")";
							}
						}
					} else if(s == "sum" || s == "prod") {
						string s1, s2, s3;
						get_latex_args(str, i2, &s1, &s2, NULL, true);
						size_t i3 = s1.find("=");
						if(i3 != string::npos) {
							s3 = s1.substr(0, i3);
							s1 = s1.substr(i3 + 1);
						}
						parse_latex_string(s1);
						parse_latex_string(s2);
						parse_latex_string(s3);
						if(s == "sum") snew = "sum(";
						else snew = "product(";
						string sarg = str.substr(i2);
						parse_latex_string(sarg);
						snew += sarg;
						snew += ",";
						snew += s1;
						snew += ",";
						snew += s2;
						if(!s3.empty()) {
							snew += ",";
							snew += s3;
						}
						snew += ")";
						i2 = str.length();
					} else if(s == "int" || s == "smallint") {
						string s1, s2, s3;
						get_latex_args(str, i2, &s1, &s2, NULL, true);
						parse_latex_string(s1);
						parse_latex_string(s2);
						snew = "integrate(";
						size_t i3 = i2;
						while(true) {
							i3 = str.find("d", i3 + 1);
							if(i3 == string::npos) break;
							if(i3 < str.length() - 1 && (str[i3 + 1] == 'x' || str[i3 + 1] == 'y' || str[i3 + 1] == 'z') && (str[i3 - 1] == ' ' || str[i3 - 1] == ')') && (i3 + 1 == str.length() - 1 || ((str[i3 + 2] < 'a' || str[i3 + 2] > 'z') && (str[i3 + 2] < 'A' || str[i3 + 2] > 'Z')))) break;
						}
						string sarg;
						if(i3 == string::npos) {
							sarg = str.substr(i2);
							i2 = str.length();
						} else {
							sarg = str.substr(i2, i3 - i2);
							s3 = str.substr(i3 + 1, 1);
							i2 = i3 + 2;
						}
						parse_latex_string(sarg);
						snew += sarg;
						snew += ",";
						snew += s1;
						snew += ",";
						snew += s2;
						if(!s3.empty()) {
							snew += ",";
							snew += s3;
						}
						snew += ")";
					} else if(s == "begin") {
						get_latex_args(str, i2, &s);
						remove_blank_ends(s);
						if(!s.empty() && s[s.length() - 1] == '*') s.erase(s.length() - 1, 1);
						if(s == "matrix" || s == "pmatrix" || s == "bmatrix" || s == "Bmatrix" || s == "vmatrix" || s == "Vmatrix" || s == "smallmatrix") {
							size_t i3 = str.find(string("\\end{") + s + "}", i);
							if(i3 == string::npos) i3 = str.length();
							string matrix = str.substr(i2, i3 - i2);
							gsub("&", ",", matrix);
							gsub("\\\\", ";", matrix);
							parse_latex_string(matrix);
							snew = "["; snew += matrix; snew += "]";
							i2 = i3;
							if(i2 < str.length()) i2 += s.length() + 6;
						} else if(s == "alignat" || s == "alignedat") {
							get_latex_args(str, i2, &s);
						} else if(s != "align" && s != "gather" && s != "aligned" && s != "gathered" && s != "equation" && s != "subequations" && s != "split" && s != "multiline" && s != "flalign" && s != "cases" && s != "eqnarray") {
							CALCULATOR->error(true, "Unsupported LaTeX command/macro %s.", (string("\\begin{") + s + "}").c_str(), NULL);
						}
					} else if(s == "end") {
						get_latex_args(str, i2, &s);
					} else if(s == "numberwithin") {
						string s2;
						get_latex_args(str, i2, &s, &s2);
					} else if(s == "num" || s == "complexnum" || s == "numproduct" || s == "numlist") {
						string opt;
						get_latex_args(str, i2, &snew, NULL, &opt);
						if(opt.find("parse-numbers=false") == string::npos) {
							read_latex_num(snew, s == "complexnum");
						} else {
							parse_latex_string(snew);
						}
					} else if(s == "ang") {
						string opt;
						get_latex_args(str, i2, &snew, NULL, &opt);
						if(opt.find("parse-numbers=false") == string::npos) {
							read_latex_num(snew, false, true);
						} else {
							parse_latex_string(snew);
						}
						snew += "deg ";
					} else if(s == "unit") {
						get_latex_args(str, i2, &snew);
						parse_latex_string(snew, true);
					} else if(s == "qty" || s == "complexqty" || s == "qtyproduct" || s == "qtylist" || s == "SI" || s == "si") {
						string s1, s2, opt;
						get_latex_args(str, i2, &s1, &s2, &opt);
						if(opt.find("parse-numbers=false") == string::npos) {
							read_latex_num(s1, s == "complexqty");
						} else {
							parse_latex_string(s1);
						}
						parse_latex_string(s2, true);
						snew = s1; snew += " "; snew += s2;
					} else {
						CALCULATOR->error(true, "Unsupported LaTeX command/macro %s.", (string("\\") + s).c_str(), NULL);
					}
				}
			}
			str.replace(i, i2 - i, snew);
			i += snew.length();
		} else {
			i++;
		}
	}
}

bool contains_underscore_symbol(const MathStructure &m) {
	if(m.isSymbolic() && !m.symbol().empty() && m.symbol()[0] == '_') return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_underscore_symbol(m[i])) return true;
	}
	return false;
}

LaTeXFunction::LaTeXFunction() : MathFunction("LaTeX", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int LaTeXFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].number().getBoolean()) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		mstruct.eval(eo);
		mstruct.set(mstruct.print(default_print_options, true, false, TAG_TYPE_LATEX), true, true);
		return 1;
	}
	string str = vargs[0].symbol();
	parse_latex_string(str);
	ParseOptions po;
	po.preserve_format = eo.parse_options.preserve_format;
	po.unknowns_enabled = true;
	bool cue = CALCULATOR->conciseUncertaintyInputEnabled();
	if(!cue) CALCULATOR->setConciseUncertaintyInputEnabled(true);
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->parse(&mstruct, str, po);
	if(contains_underscore_symbol(mstruct)) {
		CALCULATOR->endTemporaryStopMessages();
		for(size_t i = 1; i < str.size(); i++) {
			if(str[i] == '_') {
				size_t l = 1;
				while(i - l > 0 && (signed char) str[i - l] < 0 && (unsigned char) str[i - l] < 0xC0) l++;
				if((signed char) str[i - l] < 0 || (str[i - l] >= 'a' && str[i - l] <= 'z') || (str[i - l] >= 'A' && str[i - l] <= 'Z')) {
					str.insert(i - l, "\\");
					i += 2;
				}
			}
		}
		CALCULATOR->parse(&mstruct, str, po);
	} else {
		CALCULATOR->endTemporaryStopMessages(true);
	}
	if(!cue) CALCULATOR->setConciseUncertaintyInputEnabled(false);
	return 1;
}
