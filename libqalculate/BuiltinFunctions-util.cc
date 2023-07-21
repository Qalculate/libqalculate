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
using std::ostream;
using std::endl;

#define REPRESENTS_FUNCTION(x, y) x::x() : MathFunction(#y, 1) {} int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {mstruct = vargs[0]; mstruct.eval(eo); if(mstruct.y()) {mstruct.clear(); mstruct.number().setTrue();} else {mstruct.clear(); mstruct.number().setFalse();} return 1;}

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

bool create_interval(MathStructure &mstruct, const MathStructure &m1, const MathStructure &m2) {
	if(contains_infinity_v(m1) || contains_infinity_v(m2)) {
		MathStructure m1b(m1), m2b(m2);
		if(replace_infinity_v(m1b) || replace_infinity_v(m2b)) return create_interval(mstruct, m1b, m2b);
	}
	if(m1 == m2) {
		mstruct.set(m1, true);
		return 1;
	} else if(m1.isNumber() && m2.isNumber()) {
		Number nr;
		if(!nr.setInterval(m1.number(), m2.number())) return false;
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
		if(!nr.setInterval(i0 == 1 ? m1[0].number() : nr_one, i1 == 1 ? m2[0].number() : nr_one)) return 0;
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
		mstruct.set(m1, true);
		if(i0 == 1) mstruct.delChild(mstruct.size(), true);
		mstruct += nr;
		mstruct.evalSort(false);
		return true;
	} else if(m1.isMultiplication() && m1.size() == 2 && m1[0].isNumber() && m2.equals(m1[1], true)) {
		Number nr;
		if(!nr.setInterval(m1[0].number(), nr_one)) return false;
		mstruct.set(nr, true);
		mstruct *= m2;
		return true;
	} else if(m2.isMultiplication() && m2.size() == 2 && m2[0].isNumber() && m1.equals(m2[1], true)) {
		Number nr;
		if(!nr.setInterval(nr_one, m2[0].number())) return false;
		mstruct.set(nr, true);
		mstruct *= m1;
		return true;
	}
	return false;
}

IntervalFunction::IntervalFunction() : MathFunction("interval", 2) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false));
}
int IntervalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(create_interval(mstruct, vargs[0], vargs[1])) return 1;
	MathStructure marg1(vargs[0]);
	marg1.eval(eo);
	MathStructure marg2(vargs[1]);
	marg2.eval(eo);
	if(create_interval(mstruct, marg1, marg2)) return 1;
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
			return 1;
		} else if(mstruct.isNumber()) {
			mstruct.number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
			mstruct.numberUpdated();
			return 1;
		} else if(mstruct.isAddition()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isNumber()) {
					mstruct[i].number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
					mstruct[i].numberUpdated();
					mstruct.childUpdated(i + 1);
					return 1;
				}
			}
		}
		mstruct.add(m_zero, true);
		mstruct.last().number().setUncertainty(munc.number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
		mstruct.last().numberUpdated();
		mstruct.childUpdated(mstruct.size());
		return 1;
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
					return 1;
				} else if(mstruct.equals(munc[1]) || (munc[1].isFunction() && munc[1].function()->id() == FUNCTION_ID_ABS && munc[1].size() == 1 && mstruct.equals(munc[1][0]))) {
					mstruct.transform(STRUCT_MULTIPLICATION);
					mstruct.insertChild(m_one, 1);
					mstruct[0].number().setUncertainty(munc[0].number(), eo.interval_calculation == INTERVAL_CALCULATION_NONE);
					mstruct[0].numberUpdated();
					mstruct.childUpdated(1);
					return 1;
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
						return 1;
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
		return 1;
	} else {
		if(set_uncertainty(mstruct, munc, eo, true)) return 1;
		mstruct = vargs[0];
		mstruct -= vargs[1];
		mstruct.transformById(FUNCTION_ID_INTERVAL);
		MathStructure *m2 = new MathStructure(vargs[0]);
		m2->add(vargs[1]);
		mstruct.addChild_nocopy(m2);
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

ConcatenateFunction::ConcatenateFunction() : MathFunction("concatenate", 1, -1) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new TextArgument());
}
int ConcatenateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	string str;
	for(size_t i = 0; i < vargs.size(); i++) {
		str += vargs[i].symbol();
	}
	mstruct.set(str, false, true);
	return 1;
}
LengthFunction::LengthFunction() : MathFunction("len", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int LengthFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = (int) vargs[0].symbol().length();
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
	size_t i = vargs[1].symbol().find(LEFT_PARENTHESIS);
	if(i != string::npos) {
		string name = vargs[1].symbol().substr(0, i);
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
				CALCULATOR->error(false, _("A global function was deactivated. It will be restored after the new function has been removed."), NULL);
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
			CALCULATOR->error(false, _("A global unit or variable was deactivated. It will be restored after the new variable has been removed."), NULL);
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

	if(pclose(pipe) > 0 && output.empty()) {
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
	return 0;
#	endif
#else
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
#define LIST_PLOT_VALUE(y, x) if(strcmp(y, x)) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ", ";
#define LIST_PLOT_VALUE_FIRST(y, x) str += " ("; if(strcmp(y, x)) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ", ";
#define LIST_PLOT_VALUE_LAST(y, x) if(strcmp(y, x)) {str += _c(y, x); str += PLOT_OPTION_LOCALE_SEPARATOR;} str += x; str += ")\n";
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
	LIST_PLOT_OPTION_VALUES("grid", "0, 1");
	LIST_PLOT_OPTION_ALT("linewidth", "lw");
	LIST_PLOT_OPTION_ALT_WV("legend", "key"); LIST_PLOT_VALUE_FIRST("plot legend", "none"); LIST_PLOT_VALUE("plot legend", "top-left"); LIST_PLOT_VALUE("plot legend", "top-right"); LIST_PLOT_VALUE("plot legend", "bottom-left"); LIST_PLOT_VALUE("plot legend", "bottom-right"); LIST_PLOT_VALUE("plot legend", "below"); LIST_PLOT_VALUE_LAST("plot legend", "outside");
	LIST_PLOT_OPTION("title");
	LIST_PLOT_OPTION("xlabel");
	LIST_PLOT_OPTION_LAST("ylabel");
	setDescription(str);
}

int PlotFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

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
			else {
				size_t i2 = svar.rfind(SPACE);
				if(i2 == string::npos) {
					svalue = svar;
					svar = "";
					break;
				}
				svalue += svar.substr(i2);
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
	if(!mstruct.contains(mvar, true) || (!mstruct.isVector() && (!mstruct.isFunction() || (mstruct.function()->id() != FUNCTION_ID_HORZCAT && mstruct.function()->id() != FUNCTION_ID_VERTCAT)) && !mstruct.representsScalar())) {
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_HORZCAT || mstruct.function()->id() == FUNCTION_ID_VERTCAT)) mstruct.setType(STRUCT_VECTOR);
		if(!mstruct.isVector() && vargs[0].contains(mvar, true)) mstruct = vargs[0];
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
	if(mstruct.isMatrix() && mstruct.columns() == 1 && mstruct.rows() > 1) mstruct.transposeMatrix();
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

