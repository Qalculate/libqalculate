/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "util.h"
#include "MathStructure.h"
#include "MathStructure-support.h"
#include "Unit.h"
#include "Variable.h"
#include "Function.h"
#include "DataSet.h"
#include "ExpressionItem.h"
#include "Prefix.h"
#include "Number.h"

#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>

using std::string;
using std::cout;
using std::vector;
using std::endl;

#include "Calculator_p.h"

MathStructure Calculator::convertToMixedUnits(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_NONE) return mstruct;
	if(!mstruct.isMultiplication()) return mstruct;
	if(mstruct.size() != 2) return mstruct;
	size_t n_messages = messages.size();
	if(mstruct[1].isUnit() && (!mstruct[1].prefix() || mstruct[1].prefix() == decimal_null_prefix) && mstruct[0].isNumber()) {
		Prefix *p = mstruct[1].prefix();
		MathStructure mstruct_new(mstruct);
		Unit *u = mstruct[1].unit();
		Number nr = mstruct[0].number();
		if(!nr.isReal()) return mstruct;
		if(nr.isOne()) return mstruct;
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) return mstruct;
		bool negated = false;
		if(nr.isNegative()) {
			nr.negate();
			negated = true;
		}
		bool accept_obsolete = (u->subtype() == SUBTYPE_ALIAS_UNIT && abs(((AliasUnit*) u)->mixWithBase()) > 1);
		Unit *original_u = u;
		Unit *last_nonobsolete_u = u;
		Number last_nonobsolete_nr = nr;
		Number nr_one(1, 1);
		Number nr_ten(10, 1);
		MixedUnitsConversion muc = eo.mixed_units_conversion;
		while(muc > MIXED_UNITS_CONVERSION_DOWNWARDS && nr.isGreaterThan(nr_one)) {
			Unit *best_u = NULL;
			Number best_nr;
			int best_priority = 0;
			if(u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseExponent() == 1 && ((AliasUnit*) u)->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				size_t idiv = ((AliasUnit*) u)->expression().find("1" DIVISION);
				if(idiv == 0 && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS, 2) == string::npos) {
					if((((AliasUnit*) u)->mixWithBase() > 0 || (((AliasUnit*) u)->mixWithBase() == 0 && (muc == MIXED_UNITS_CONVERSION_FORCE_INTEGER || muc == MIXED_UNITS_CONVERSION_FORCE_ALL))) && (((AliasUnit*) u)->mixWithBaseMinimum() <= 1 || nr.isGreaterThanOrEqualTo(((AliasUnit*) u)->mixWithBaseMinimum()))) {
						best_u = ((AliasUnit*) u)->firstBaseUnit();
						MathStructure mstruct_nr(nr);
						MathStructure m_exp(m_one);
						((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
						mstruct_nr.eval(eo);
						if(!mstruct_nr.isNumber() || !m_exp.isOne() || !mstruct_nr.number().isLessThan(nr) || !mstruct_nr.number().isGreaterThanOrEqualTo(nr_one)) {
							best_u = NULL;
						} else {
							best_nr = mstruct_nr.number();
							best_priority = ((AliasUnit*) u)->mixWithBase();
						}
					}
				}
			}
			for(size_t i = 0; i < units.size(); i++) {
				Unit *ui = units[i];
				if(ui->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) ui)->firstBaseUnit() == u  && ((AliasUnit*) ui)->firstBaseExponent() == 1) {
					AliasUnit *aui = (AliasUnit*) ui;
					int priority_i = aui->mixWithBase();
					if(((priority_i > 0 && (!best_u || priority_i <= best_priority)) || (best_priority == 0 && priority_i == 0 && ((muc == MIXED_UNITS_CONVERSION_FORCE_INTEGER && aui->expression().find_first_not_of(NUMBERS) == string::npos) || muc == MIXED_UNITS_CONVERSION_FORCE_ALL))) && (aui->mixWithBaseMinimum() <= 1 || nr.isGreaterThanOrEqualTo(aui->mixWithBaseMinimum()))) {
						MathStructure mstruct_nr(nr);
						MathStructure m_exp(m_one);
						aui->convertFromFirstBaseUnit(mstruct_nr, m_exp);
						mstruct_nr.eval(eo);
						if(mstruct_nr.isNumber() && m_exp.isOne() && mstruct_nr.number().isLessThan(nr) && mstruct_nr.number().isGreaterThanOrEqualTo(nr_one) && (!best_u || mstruct_nr.number().isLessThan(best_nr))) {
							best_u = ui;
							best_nr = mstruct_nr.number();
							best_priority = priority_i;
						}
					}
				}
			}
			if(!best_u) break;
			if(best_priority != 0 && muc > MIXED_UNITS_CONVERSION_DEFAULT) muc = MIXED_UNITS_CONVERSION_DEFAULT;
			u = best_u;
			nr = best_nr;
			if(accept_obsolete || best_priority <= 1) {
				last_nonobsolete_u = u;
				last_nonobsolete_nr = nr;
			}
		}
		u = last_nonobsolete_u;
		nr = last_nonobsolete_nr;
		if(u != original_u) {
			if(negated) last_nonobsolete_nr.negate();
			mstruct_new[0].set(last_nonobsolete_nr);
			mstruct_new[1].set(u, p);
		}
		while((u->subtype() == SUBTYPE_BASE_UNIT || (u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT && ((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || muc == MIXED_UNITS_CONVERSION_FORCE_ALL || muc == MIXED_UNITS_CONVERSION_FORCE_INTEGER))) && !nr.isInteger() && nr.isNonZero()) {
			Number int_nr(nr);
			int_nr.intervalToMidValue();
			int_nr.trunc();
			if(muc == MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP && int_nr.isZero()) break;
			nr -= int_nr;
			bool b = false;
			Number best_nr;
			Unit *best_u = NULL;
			bool non_int = false;
			if(u->subtype() == SUBTYPE_ALIAS_UNIT && (muc == MIXED_UNITS_CONVERSION_FORCE_ALL || (((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos))) {
				MathStructure mstruct_nr(nr);
				MathStructure m_exp(m_one);
				((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
				mstruct_nr.eval(eo);
				while(!accept_obsolete && ((AliasUnit*) u)->firstBaseUnit()->subtype() == SUBTYPE_ALIAS_UNIT && abs(((AliasUnit*) ((AliasUnit*) u)->firstBaseUnit())->mixWithBase()) > 1) {
					u = ((AliasUnit*) u)->firstBaseUnit();
					if(((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || muc == MIXED_UNITS_CONVERSION_FORCE_ALL || (muc == MIXED_UNITS_CONVERSION_FORCE_INTEGER && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos))) {
						((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
						mstruct_nr.eval(eo);
						if(!mstruct_nr.isNumber() || !m_exp.isOne()) break;
					} else {
						mstruct_nr.setUndefined();
						break;
					}
				}
				if(mstruct_nr.isNumber() && m_exp.isOne()) {
					if(mstruct_nr.number().isLessThanOrEqualTo(nr)) {
						if(muc == MIXED_UNITS_CONVERSION_FORCE_ALL) {
							best_u = ((AliasUnit*) u)->firstBaseUnit();
							best_nr = mstruct_nr.number();
							non_int = true;
						}
					} else {
						if(((AliasUnit*) u)->mixWithBase() != 0 && muc > MIXED_UNITS_CONVERSION_DEFAULT) muc = MIXED_UNITS_CONVERSION_DEFAULT;
						u = ((AliasUnit*) u)->firstBaseUnit();
						nr = mstruct_nr.number();
						b = true;
					}
				}
			}
			if(!b) {
				Number best_nr;
				int best_priority = 0;
				for(size_t i = 0; i < units.size(); i++) {
					Unit *ui = units[i];
					if(ui->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) ui)->firstBaseUnit() == u && ((AliasUnit*) ui)->firstBaseExponent() == 1) {
						AliasUnit *aui = (AliasUnit*) ui;
						int priority_i = aui->mixWithBase();
						if(aui->expression().find("1" DIVISION) == 0 && aui->expression().find_first_not_of(NUMBERS DIVISION, 2) == string::npos && ((priority_i > 0 && (!best_u || priority_i <= best_priority)) || (best_priority == 0 && priority_i == 0 && (muc == MIXED_UNITS_CONVERSION_FORCE_INTEGER || muc == MIXED_UNITS_CONVERSION_FORCE_ALL)))) {
							MathStructure mstruct_nr(nr);
							MathStructure m_exp(m_one);
							aui->convertFromFirstBaseUnit(mstruct_nr, m_exp);
							mstruct_nr.eval(eo);
							if(mstruct_nr.isNumber() && m_exp.isOne() && mstruct_nr.number().isGreaterThan(nr) && mstruct_nr.number().isGreaterThanOrEqualTo(nr_one) && (!best_u || non_int || mstruct_nr.number().isLessThan(best_nr))) {
								non_int = false;
								best_u = ui;
								best_nr = mstruct_nr.number();
								best_priority = priority_i;
							}
						}
					}
				}
				if(!best_u) break;
				if(best_priority != 0 && muc > MIXED_UNITS_CONVERSION_DEFAULT) muc = MIXED_UNITS_CONVERSION_DEFAULT;
				u = best_u;
				nr = best_nr;
			}
			MathStructure mstruct_term;
			if(negated) {
				Number pos_nr(nr);
				pos_nr.negate();
				mstruct_term.set(pos_nr);
			} else {
				mstruct_term.set(nr);
			}
			mstruct_term *= MathStructure(u, p);
			if(int_nr.isZero()) {
				if(mstruct_new.isAddition()) mstruct_new[mstruct_new.size() - 1].set(mstruct_term);
				else mstruct_new.set(mstruct_term);
			} else {
				if(negated) int_nr.negate();
				if(mstruct_new.isAddition()) mstruct_new[mstruct_new.size() - 1][0].set(int_nr);
				else mstruct_new[0].set(int_nr);
				mstruct_new.add(mstruct_term, true);
			}
		}
		cleanMessages(mstruct_new, n_messages + 1);
		return mstruct_new;
	}
	return mstruct;
}

MathStructure Calculator::convert(double value, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo) {
	size_t n_messages = messages.size();
	MathStructure mstruct(value);
	mstruct *= from_unit;
	mstruct.eval(eo);
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	mstruct.eval(eo);
	cleanMessages(mstruct, n_messages + 1);
	return mstruct;

}
MathStructure Calculator::convert(string str, Unit *from_unit, Unit *to_unit, int msecs, const EvaluationOptions &eo) {
	return convertTimeOut(str, from_unit, to_unit, msecs, eo);
}
MathStructure Calculator::convertTimeOut(string str, Unit *from_unit, Unit *to_unit, int msecs, const EvaluationOptions &eo) {
	MathStructure mstruct;
	parse(&mstruct, str, eo.parse_options);
	mstruct *= from_unit;
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) return mstruct;
	bool had_msecs = msecs > 0;
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	tmp_tostruct = NULL;
	bool b_parse = false;
	if(!calculate_thread->write(b_parse)) {calculate_thread->cancel(); return mstruct;}
	void *x = (void*) &mstruct;
	if(!calculate_thread->write(x)) {calculate_thread->cancel(); return mstruct;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		abort();
		mstruct.setAborted();
		return mstruct;
	}
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	b_busy = true;
	if(!calculate_thread->write(b_parse)) {calculate_thread->cancel(); return mstruct;}
	x = (void*) &mstruct;
	if(!calculate_thread->write(x)) {calculate_thread->cancel(); return mstruct;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		abort();
		mstruct.setAborted();
	}
	return mstruct;
}
MathStructure Calculator::convert(string str, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo) {
	size_t n_messages = messages.size();
	MathStructure mstruct;
	parse(&mstruct, str, eo.parse_options);
	mstruct *= from_unit;
	mstruct.eval(eo);
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	mstruct.eval(eo);
	cleanMessages(mstruct, n_messages + 1);
	return mstruct;
}
MathStructure Calculator::convert(const MathStructure &mstruct, KnownVariable *to_var, const EvaluationOptions &eo) {
	if(mstruct.contains(to_var, true) > 0) return mstruct;
	size_t n_messages = messages.size();
	if(!to_var->unit().empty() && to_var->isExpression()) {
		int b = mstruct.containsRepresentativeOfType(STRUCT_UNIT, true, true);
		if(b > 0 || (b < 0 && b_var_units)) {
			beginTemporaryStopMessages();
			CompositeUnit cu("", "temporary_composite_convert", "", to_var->unit());
			if(!CALCULATOR->endTemporaryStopMessages() && cu.countUnits() > 0) {
				AliasUnit au("", "temporary_alias_convert", "", "", "", &cu, to_var->expression());
				bool unc_rel = false;
				if(!to_var->uncertainty(&unc_rel).empty()) au.setUncertainty(to_var->uncertainty(), unc_rel);
				au.setApproximate(to_var->isApproximate());
				au.setPrecision(to_var->precision());
				MathStructure mstruct_new(convert(mstruct, &au, eo, false, false));
				if(mstruct_new.contains(&au)) {
					mstruct_new.replace(&au, to_var);
					if(b_var_units || !mstruct_new.containsType(STRUCT_UNIT, true)) return mstruct_new;
				}
			}
		} else if(b_var_units) {
			MathStructure mstruct_new(mstruct);
			bool b_var_units_bak = b_var_units;
			b_var_units = false;
			mstruct_new /= to_var->get();
			b_var_units = b_var_units_bak;
			mstruct_new.eval(eo);
			mstruct_new.multiply(to_var, true);
			cleanMessages(mstruct, n_messages + 1);
			return mstruct_new;
		}
	}
	MathStructure mstruct_new(mstruct);
	mstruct_new /= to_var->get();
	mstruct_new.eval(eo);
	if((eo.auto_post_conversion == POST_CONVERSION_OPTIMAL || eo.auto_post_conversion == POST_CONVERSION_OPTIMAL_SI) && mstruct_new.containsType(STRUCT_UNIT, true)) {
		mstruct_new.set(CALCULATOR->convertToOptimalUnit(mstruct_new, eo, false));
	}
	mstruct_new.multiply(to_var, true);
	cleanMessages(mstruct, n_messages + 1);
	return mstruct_new;
}
long int count_unit_powers(const MathStructure &m) {
	if(m.isPower() && m[0].isUnit() && m[1].isInteger()) {
		long int exp = m[1].number().lintValue();
		if(exp < 0) return -exp;
		return exp;
	}
	if(m.isUnit()) return 1;
	long int exp = 0;
	for(size_t i = 0; i < m.size(); i++) {
		exp += count_unit_powers(m[i]);
	}
	return exp;
}
void fix_to_struct(MathStructure &m) {
	if(m.isPower() && m[0].isUnit()) {
		if(m[0].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m[0].setUnit(u);
		}
		if(m[0].prefix() == NULL && m[0].unit()->defaultPrefix() != 0) {
			m[0].setPrefix(CALCULATOR->getExactDecimalPrefix(m[0].unit()->defaultPrefix()));
		}
	} else if(m.isUnit()) {
		if(m.unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m.setUnit(u);
		}
		if(m.prefix() == NULL && m.unit()->defaultPrefix() != 0) {
			m.setPrefix(CALCULATOR->getExactDecimalPrefix(m.unit()->defaultPrefix()));
		}
	} else {
		for(size_t i = 0; i < m.size();) {
			if(m[i].isUnit()) {
				if(m[i].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i].setUnit(u);
				}
				if(m[i].prefix() == NULL && m[i].unit()->defaultPrefix() != 0) {
					m[i].setPrefix(CALCULATOR->getExactDecimalPrefix(m[i].unit()->defaultPrefix()));
				}
				i++;
			} else if(m[i].isPower() && m[i][0].isUnit()) {
				if(m[i][0].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i][0].setUnit(u);
				}
				if(m[i][0].prefix() == NULL && m[i][0].unit()->defaultPrefix() != 0) {
					m[i][0].setPrefix(CALCULATOR->getExactDecimalPrefix(m[i][0].unit()->defaultPrefix()));
				}
				i++;
			} else {
				m.delChild(i + 1);
			}
		}
		if(m.size() == 0) m.clear();
		if(m.size() == 1) m.setToChild(1);
	}
}
void fix_to_struct2(MathStructure &m) {
	if(m.isPower() && m[0].isUnit()) {
		m[0].setPrefix(NULL);
	} else if(m.isUnit()) {
		m.setPrefix(NULL);
	} else {
		for(size_t i = 0; i < m.size();) {
			if(m[i].isUnit()) {
				m[i].setPrefix(NULL);
				i++;
			} else if(m[i].isPower() && m[i][0].isUnit()) {
				m[i][0].setPrefix(NULL);
				i++;
			} else {
				m.delChild(i + 1);
			}
		}
		if(m.size() == 0) m.clear();
		if(m.size() == 1) m.setToChild(1);
	}
}
MathStructure Calculator::convert(const MathStructure &mstruct, Unit *to_unit, const EvaluationOptions &eo, bool always_convert, bool convert_to_mixed_units) {
	return convert(mstruct, to_unit, eo, always_convert, convert_to_mixed_units, false, NULL);
}
Unit *find_ounce(const MathStructure &m) {
	if(m.isUnit() && m.unit()->referenceName() == "oz") return m.unit();
	for(size_t i = 0; i < m.size(); i++) {
		Unit *u = find_ounce(m[i]);
		if(u) return u;
	}
	return NULL;
}

bool contains_angle_returning_function(const MathStructure &m) {
	if(m.isFunction() && (m.function()->id() == FUNCTION_ID_ATAN || m.function()->id() == FUNCTION_ID_ACOS || m.function()->id() == FUNCTION_ID_ASIN || m.function()->id() == FUNCTION_ID_ARG || m.function()->id() == FUNCTION_ID_ATAN2 || m.function()->id() == FUNCTION_ID_RADIANS_TO_DEFAULT_ANGLE_UNIT)) return true;
	if(m.isFunction() && m.function()->subtype() == SUBTYPE_USER_FUNCTION) {
		UserFunction *f = (UserFunction*) m.function();
		if(f->formula().find("arctan") != string::npos || f->formula().find("arccos") != string::npos || f->formula().find("arcsin") != string::npos || f->formula().find("atan(") != string::npos || f->formula().find("acos(") != string::npos || f->formula().find("asin(") != string::npos) return true;
	}
	if(m.isVariable() && m.variable()->isKnown()) return contains_angle_returning_function(((KnownVariable*) m.variable())->get());
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_angle_returning_function(m[i])) return true;
	}
	return false;
}

void contains_angle_ratio_b(const MathStructure &m, bool &num, bool &den, bool in_den) {
	if(m.isUnit() && m.unit()->baseUnit()->referenceName() == "m") {
		if(in_den) den = true;
		else num = true;
	}
	if(num && den) return;
	if(m.isPower()) {
		if(m[1].representsNegative()) in_den = !in_den;
		contains_angle_ratio_b(m[0], num, den, in_den);
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			if((i == 0 && m.isInverse()) || (i == 1 && m.isDivision())) in_den = !in_den;
			contains_angle_ratio_b(m[i], num, den, in_den);
			if(num && den) return;
		}
	}
}
bool contains_angle_ratio(const MathStructure &m) {
	if(m.isAddition()) {
		for(size_t i = 0; i < m.size(); i++) {
			bool num = false, den = false;
			contains_angle_ratio_b(m[i], num, den, false);
			if(num && den) return true;
		}
		return false;
	}
	bool num = false, den = false;
	contains_angle_ratio_b(m, num, den, false);
	return num && den;
}

MathStructure get_units_for_parsed_expression(const MathStructure *parsed_struct, Unit *to_unit, const EvaluationOptions &eo, const MathStructure *mstruct = NULL) {
	CompositeUnit *cu = NULL;
	if(to_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) cu = (CompositeUnit*) to_unit;
	if(cu && cu->countUnits() == 0) return m_zero;
	if((mstruct && mstruct->containsType(STRUCT_UNIT, true)) || (!mstruct && parsed_struct->containsType(STRUCT_UNIT, false, true, true))) return m_zero;
	int exp1, exp2;
	bool b_ratio = cu && cu->countUnits() == 2 && cu->get(1, &exp1)->baseUnit() == cu->get(2, &exp2)->baseUnit() && exp1 == -exp2;
	bool b_angle = to_unit->baseUnit() == CALCULATOR->getRadUnit() || (b_ratio && cu->get(1)->baseUnit()->referenceName() == "m" && (exp1 == 1 || exp2 == 1));
	for(size_t i = 1; !b_angle && cu && i <= cu->countUnits(); i++) {
		 if(cu->get(i)->baseUnit() == CALCULATOR->getRadUnit()) b_angle = true;
	}
	if(!b_ratio || b_angle) {
		if(mstruct) {
			int ct = 0;
			ct = parsed_struct->containsType(STRUCT_UNIT, false, true, true);
			if(!(ct == 0 || (b_angle && (ct < 0 || (parsed_struct && !contains_angle_ratio(*parsed_struct)))))) return m_zero;
		}
		EvaluationOptions eo2 = eo;
		if(eo.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
		MathStructure munit(to_unit);
		munit.unformat();
		if(b_angle && (b_ratio || (!cu && to_unit->baseExponent() == 1) || (cu && cu->get(1, &exp1) && exp1 == 1))) {
			munit = default_angle_unit(eo, true);
		} else {
			munit = CALCULATOR->convertToOptimalUnit(munit, eo2, true);
			if(b_angle && !DEFAULT_RADIANS(eo.parse_options.angle_unit)) munit.replace(CALCULATOR->getRadUnit(), default_angle_unit(eo, true));
			munit.unformat();
		}
		fix_to_struct(munit);
		return munit;
	}
	return m_zero;
}

bool contains_part_of_unit(const MathStructure &m, Unit *u) {
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		for(size_t i = 1; i <= ((CompositeUnit*) u)->countUnits(); i++) {
			if(contains_part_of_unit(m, ((CompositeUnit*) u)->get(i))) return true;
		}
		return false;
	}
	if(m.isUnit()) {
		if(m.unit() == u) return true;
		if(m.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			for(size_t i = 1; i <= ((CompositeUnit*) m.unit())->countUnits(); i++) {
				if(((CompositeUnit*) m.unit())->get(i) == u) return true;
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_part_of_unit(m[i], u)) return true;
	}
	return false;
}

void replace_hz(MathStructure &m) {
	if(m.isUnit() && m.unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) m.unit())->firstBaseExponent() == -1 && ((AliasUnit*) m.unit())->expression() == "1") {
		m.setUnit(((AliasUnit*) m.unit())->firstBaseUnit());
		m.raise(m_minus_one);
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_hz(m[i]);
	}
}

void test_convert(MathStructure &mstruct_new, Unit *to_unit, long int &n, bool b_pos, EvaluationOptions &eo2) {
	if(n <= 0 || (eo2.auto_post_conversion != POST_CONVERSION_OPTIMAL && eo2.auto_post_conversion != POST_CONVERSION_OPTIMAL_SI) || CALCULATOR->aborted()) return;
	AutoPostConversion pc = eo2.auto_post_conversion;
	eo2.auto_post_conversion = POST_CONVERSION_NONE;
	MathStructure mtest = CALCULATOR->convertToOptimalUnit(mstruct_new, eo2, pc == POST_CONVERSION_OPTIMAL_SI);
	long int ntest = count_unit_powers(mtest);
	if(!contains_part_of_unit(mtest, to_unit) && (pc == POST_CONVERSION_OPTIMAL_SI || ntest < n)) {
		mstruct_new = mtest;
		if(b_pos) replace_hz(mstruct_new);
		n = ntest;
	}
	if(b_pos && n > 1) {
		MathStructure mtest(mstruct_new);
		mtest.inverse();
		mtest.eval(eo2);
		mtest = CALCULATOR->convertToOptimalUnit(mtest, eo2, false);
		long int ntest = count_unit_powers(mtest);
		if(!contains_part_of_unit(mtest, to_unit) && ntest < n) {
			replace_hz(mtest);
			eo2.sync_units = false;
			mtest.inverse();
			mtest.eval(eo2);
			eo2.sync_units = true;
			mstruct_new = mtest;
			n = ntest;
		}
	}
	eo2.auto_post_conversion = pc;
}

MathStructure Calculator::convert(const MathStructure &mstruct, Unit *to_unit, const EvaluationOptions &eo, bool always_convert, bool convert_to_mixed_units, bool transform_orig, MathStructure *parsed_struct) {
	CompositeUnit *cu = NULL;
	if(to_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) cu = (CompositeUnit*) to_unit;
	if(cu && cu->countUnits() == 0) return mstruct;
	int exp1, exp2;
	bool b_ratio = cu && cu->countUnits() == 2 && cu->get(1, &exp1)->baseUnit() == cu->get(2, &exp2)->baseUnit() && exp1 == -exp2;
	bool b_angle = to_unit->baseUnit() == getRadUnit() || (b_ratio && cu->get(1)->baseUnit()->referenceName() == "m" && (exp1 == 1 || exp2 == 1));
	for(size_t i = 1; !b_angle && cu && i <= cu->countUnits(); i++) {
		 if(cu->get(i)->baseUnit() == getRadUnit()) b_angle = true;
	}
	if((!b_ratio || b_angle) && !mstruct.containsType(STRUCT_UNIT, true)) {
		int ct = 0;
		if(parsed_struct) ct = parsed_struct->containsType(STRUCT_UNIT, false, true, true);
		if(transform_orig && (ct == 0 || (b_angle && (ct < 0 || (parsed_struct && !contains_angle_ratio(*parsed_struct)))))) {
			// multiply original value with base units
			EvaluationOptions eo2 = eo;
			if(eo.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
			MathStructure munit(to_unit);
			munit.unformat();
			if(b_angle && (b_ratio || (!cu && to_unit->baseExponent() == 1) || (cu && cu->get(1, &exp1) && exp1 == 1))) {
				munit = default_angle_unit(eo, true);
			} else {
				munit = convertToOptimalUnit(munit, eo2, true);
				if(b_angle && !DEFAULT_RADIANS(eo.parse_options.angle_unit)) munit.replace(getRadUnit(), default_angle_unit(eo, true));
				munit.unformat();
			}
			fix_to_struct(munit);
			if(!munit.isZero()) {
				MathStructure mstruct_new(mstruct);
				mstruct_new *= munit;
				PrintOptions po = message_printoptions;
				po.negative_exponents = false;
				munit.format(po);
				if(munit.isMultiplication() && munit.size() >= 2) {
					if(munit[0].isOne()) munit.delChild(1, true);
					else if(munit[1].isOne()) munit.delChild(2, true);
				}
				if(parsed_struct) parsed_struct->multiply(munit, true);
				return convert(mstruct_new, to_unit, eo, always_convert, convert_to_mixed_units, false, NULL);
			}
		}
		if(!b_angle) return mstruct;
	}
	MathStructure mstruct_new(mstruct);
	size_t n_messages = messages.size();
	if(to_unit->hasNonlinearRelationTo(to_unit->baseUnit()) && to_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		mstruct_new = convert(mstruct, to_unit->baseUnit(), eo, always_convert, convert_to_mixed_units, transform_orig, parsed_struct);
		mstruct_new.calculateDivide(((CompositeUnit*) to_unit->baseUnit())->generateMathStructure(false, eo.keep_prefixes), eo);
		to_unit->convertFromBaseUnit(mstruct_new);
		mstruct_new.eval(eo);
		mstruct_new.multiply(MathStructure(to_unit, eo.keep_prefixes ? decimal_null_prefix : NULL));
		EvaluationOptions eo2 = eo;
		eo2.sync_units = false;
		eo2.keep_prefixes = true;
		mstruct_new.eval(eo2);
		cleanMessages(mstruct, n_messages + 1);
		return mstruct_new;
	}
	//bool b_simple = !cu && (to_unit->subtype() != SUBTYPE_ALIAS_UNIT || (((AliasUnit*) to_unit)->baseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT && ((AliasUnit*) to_unit)->baseExponent() == 1));

	bool b_changed = false;
	if(mstruct_new.isAddition()) {
		if(aborted()) return mstruct;
		mstruct_new.factorizeUnits();
		if(!b_changed && !mstruct_new.equals(mstruct, true, true)) b_changed = true;
	}

	if(mstruct_new.size() > 0 && !mstruct_new.isPower() && !mstruct_new.isUnit() && !mstruct_new.isMultiplication()) {
		if(mstruct_new.size() > 0) {
			for(size_t i = 0; i < mstruct_new.size(); i++) {
				if(aborted()) return mstruct;
				if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_FREE || (!mstruct_new.function()->getArgumentDefinition(i + 1)->tests() && mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE)) {
					mstruct_new[i] = convert(mstruct_new[i], to_unit, eo, false, convert_to_mixed_units, false, NULL);
					if(!b_changed && !mstruct_new.equals(mstruct[i], true, true)) b_changed = true;
				}
			}
			if(b_changed && !b_ratio && !mstruct.isFunction()) {
				mstruct_new.childrenUpdated();
				EvaluationOptions eo2 = eo;
				//eo2.calculate_functions = false;
				eo2.sync_units = false;
				eo2.keep_prefixes = true;
				mstruct_new.eval(eo2);
				cleanMessages(mstruct, n_messages + 1);
			}
			return mstruct_new;
		}
	} else {
		EvaluationOptions eo2 = eo;
		eo2.keep_prefixes = true;
		bool b = false;
		if(eo.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
		if(transform_orig) {
			Unit *bu = to_unit->baseUnit();
			// 1 = J, 2 = K, 3 = 1/m, 4 = Hz
			int u1_type = 0;
			int exp = to_unit->baseExponent();
			if(to_unit->referenceName() == "oz") {
				u1_type = 6;
			} else if(exp == 1) {
				if(bu->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					CompositeUnit *cu2 = (CompositeUnit*) bu;
					if(cu2->countUnits() == 1) {
						bu = cu2->get(1, &exp);
						exp *= bu->baseExponent();
						bu = bu->baseUnit();
						if(exp == -1 && bu->referenceName() == "m") {
							u1_type = 3;
						} else if(exp == -1 && bu->referenceName() == "s") {
							u1_type = 4;
						}
					} else if(cu2->countUnits() == 2 && cu2->referenceName() == "N_m") {
						u1_type = 1;
					}
					if(u1_type == 0) {
						for(size_t i = 1; i <= cu2->countUnits(); i++) {
							if(to_unit == cu2 && cu2->get(i)->referenceName() == "oz" && !to_unit->isRegistered()) {
								u1_type = 6;
								break;
							}
						}
						for(size_t i = 1; i <= cu2->countUnits(); i++) {
							bu = cu2->get(i, &exp);
							exp *= bu->baseExponent();
							bu = bu->baseUnit();
							if((u1_type == 0 || u1_type == 6) && exp % 3 == 0 && bu->referenceName() == "m") {
								if(u1_type == 6) {u1_type = 0; break;}
								u1_type = 5;
							} else if(u1_type != 6 && bu->referenceName() == "g") {
								u1_type = 0;
								break;
							}
						}
					}
				} else if(bu->referenceName() == "K") {
					u1_type = 2;
				}
			} else if(exp == -1) {
				if(bu->referenceName() == "m") {
					u1_type = 3;
				} else if(bu->referenceName() == "s") {
					u1_type = 4;
				}
			} else if(exp % 3 == 0) {
				if(bu->referenceName() == "m") {
					u1_type = 5;
				}
			}
			if(u1_type == 5) {
				Unit *u1 = find_ounce(mstruct_new);
				if(u1 && (!parsed_struct || find_ounce(*parsed_struct))) {
					Unit *u_gram = getActiveUnit("g");
					if(!to_unit->containsRelativeTo(u_gram)) {
						Unit *u2 = getActiveUnit("fl_oz");
						if(u2) {
							if(parsed_struct) parsed_struct->replace(u1, u2);
							mstruct_new.replace(u1, u2);
						}
					}
				}
			} else if(u1_type > 0) {
				const MathStructure *mstruct_u = NULL;
				if(mstruct_new.isUnit_exp()) {
					mstruct_u = &mstruct_new;
				} else if(mstruct_new.isMultiplication() && mstruct_new.size() >= 2 && mstruct_new.last().isUnit_exp() && !mstruct_new[mstruct_new.size() - 2].isUnit_exp()) {
					mstruct_u = &mstruct_new.last();
				}
				if(mstruct_u && (!mstruct_u->isPower() || (u1_type == 6 && (*mstruct_u)[1].isInteger()) || (u1_type == 1 && (*mstruct_u)[1].isMinusOne()))) {
					if(mstruct_u->isPower()) bu = (*mstruct_u)[0].unit();
					else bu = mstruct_u->unit();
					exp = bu->baseExponent();
					if(mstruct_u->isPower()) {
						if(u1_type == 6) exp *= (*mstruct_u)[1].number().intValue();
						else exp = -exp;
					}
					bu = bu->baseUnit();
					int u2_type = 0;
					if(exp == 1) {
						if(bu->subtype() == SUBTYPE_COMPOSITE_UNIT) {
							CompositeUnit *cu2 = (CompositeUnit*) bu;
							if(u1_type == 1 && cu2->countUnits() == 1) {
								bu = cu2->get(1, &exp);
								exp *= bu->baseExponent();
								bu = bu->baseUnit();
								if(exp == -1 && bu->referenceName() == "m") {
									u2_type = 3;
								} else if(exp == -1 && bu->referenceName() == "s") {
									u2_type = 4;
								}
							} else if(u1_type != 1 && cu2->countUnits() == 2 && cu2->referenceName() == "N_m") {
								u2_type = 1;
							}
							if(u2_type == 0 && u1_type == 6) {
								for(size_t i = 1; i <= cu2->countUnits(); i++) {
									bu = cu2->get(i, &exp);
									exp *= bu->baseExponent();
									bu = bu->baseUnit();
									if(u2_type == 0 && exp % 3 == 0 && bu->referenceName() == "m") {
										u2_type = 5;
									} else if(bu->referenceName() == "g") {
										u2_type = 0;
										break;
									}
								}
							}
						} else if(u1_type == 1 && bu->referenceName() == "K") {
							u2_type = 2;
						}
					} else if(exp == -1 && u1_type == 1) {
						if(bu->referenceName() == "m") {
							u2_type = 3;
						} else if(bu->referenceName() == "s") {
							u2_type = 4;
						}
					} else if(exp % 3 == 0 && u1_type == 6 && bu->referenceName() == "m") {
						u2_type = 5;
					}
					Variable *v = NULL;
					if(u1_type == 1) {
						if(u2_type == 2) v = getActiveVariable("K_to_J");
						else if(u2_type == 3) v = getActiveVariable("m_to_J");
						else if(u2_type == 4) v = getActiveVariable("Hz_to_J");
					} else if(u2_type == 1) {
						if(u1_type == 2) v = getActiveVariable("J_to_K");
						else if(u1_type == 3) v = getActiveVariable("J_to_m");
						else if(u1_type == 4) v = getActiveVariable("J_to_Hz");
					} else if(u1_type == 6 && u2_type == 5) {
						Unit *u = getActiveUnit("fl_oz");
						if(u) {
							if(to_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
								CompositeUnit *cu2 = (CompositeUnit*) to_unit;
								for(size_t i = 1; i <= cu2->countUnits(); i++) {
									bu = cu2->get(i, &exp);
									if(bu->referenceName() == "oz") {
										cu2->del(i);
										cu2->add(u, exp);
										break;
									}
								}
							} else {
								to_unit = u;
							}
						}
					}
					if(v) {
						mstruct_new *= v;
						if(parsed_struct) parsed_struct->multiply(v, true);
					}
				}
			}
		}
		if(mstruct_new.convert(to_unit, true, NULL, false, eo2, eo.keep_prefixes ? decimal_null_prefix : NULL)) {
			b = true;
		} else if(b_ratio) {
			int exp;
			if(cu->get(1, &exp)->baseUnit()->referenceName() == "m") {
				Unit *u = NULL;
				if(exp == 1 || exp == -1) u = getRadUnit();
				else if(exp == 2 || exp == -2) u = getActiveUnit("sr");
				if(u) {
					mstruct_new.convert(u, true, NULL, false, eo2, NULL);
					if(mstruct_new.contains(u, false, false, false)) mstruct_new.replace(u, m_one, false, false);
				}
			}
			b = true;
		} else if(to_unit->baseUnit() == getRadUnit() && mstruct_new.contains(to_unit, true) < 1) {
			mstruct_new.multiply(getRadUnit(), true);
			if(to_unit->baseExponent() != 1) mstruct_new.last().raise(to_unit->baseExponent());
			b = mstruct_new.convert(to_unit, true, NULL, false, eo2, eo.keep_prefixes ? decimal_null_prefix : NULL) || always_convert;
		} else if(always_convert) {
			b = true;
		} else {
			CompositeUnit *cu2 = cu;
			if(to_unit->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) to_unit)->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				cu2 = (CompositeUnit*) ((AliasUnit*) to_unit)->baseUnit();
			}
			if(cu2) {
				switch(mstruct_new.type()) {
					case STRUCT_UNIT: {
						if(cu2->containsRelativeTo(mstruct_new.unit())) {
							b = true;
						}
						break;
					}
					case STRUCT_MULTIPLICATION: {
						flattenMultiplication(mstruct_new);
						for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
							if(aborted()) return mstruct;
							if(mstruct_new.getChild(i)->isUnit() && cu2->containsRelativeTo(mstruct_new.getChild(i)->unit())) {
								b = true;
							}
							if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && cu2->containsRelativeTo(mstruct_new.getChild(i)->base()->unit())) {
								b = true;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mstruct_new.base()->isUnit() && cu2->containsRelativeTo(mstruct_new.base()->unit())) {
							b = true;
						}
						break;
					}
					default: {}
				}
			}
		}
		if(!b) {
			eo2.sync_units = true;
			eo2.keep_prefixes = false;
			mstruct_new.eval(eo2);
			eo2.keep_prefixes = true;
			if(mstruct_new.convert(to_unit, true, NULL, false, eo2, eo.keep_prefixes ? decimal_null_prefix : NULL)) {
				b = true;
			}
		}
		if(b) {

			eo2.approximation = eo.approximation;
			eo2.sync_units = true;
			eo2.keep_prefixes = false;

			bool b_eval = true;
			if(to_unit != priv->u_celsius && to_unit != priv->u_fahrenheit) {
				int exp = 1;
				MathStructure mbak(mstruct_new);
				mstruct_new.divide_nocopy(new MathStructure(to_unit, NULL));
				mstruct_new.eval(eo2);
				eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
				bool b_pos = !cu, b_neg = false;
				if(cu || to_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					CompositeUnit *cu2 = cu;
					if(cu2) b_pos = true;
					else cu2 = (CompositeUnit*) to_unit->baseUnit();
					int exp2 = 1;
					for(size_t i = 1; i <= cu2->countUnits(); i++) {
						cu2->get(i, &exp2);
						if(exp2 < 0) b_neg = true;
						else b_pos = true;
					}
				}
				long int n = count_unit_powers(mstruct_new);
				test_convert(mstruct_new, to_unit, n, b_pos, eo2);
				if(n > 0) {
					MathStructure mtest(mbak);
					MathStructure mtest2;
					if(b_pos && b_neg) {
						mtest.inverse();
						mtest.divide_nocopy(new MathStructure(to_unit, NULL));
						mtest.eval(eo2);
						if(!mtest.containsType(STRUCT_UNIT)) {
							mstruct_new = mtest;
							n = 0;
						}
					}
					bool prio_power = transform_orig;
					if(n > 0 && !prio_power) {
						if(cu) {
							for(size_t i = 1; i <= cu->countUnits(); i++) {
								if(!cu->get(i)->isSIUnit()) {
									prio_power = true;
									break;
								}
							}
						} else {
							prio_power = !to_unit->isSIUnit();
						}
					}
					if(n > 0 && (!cu || (cu->countUnits() == 1 && (cu->get(1, &exp) && exp == 1)))) {
						if(b_pos && b_neg) mtest = mbak;
						while(exp > -10 && n > 0) {
							mtest.multiply_nocopy(new MathStructure(to_unit, NULL));
							mtest.eval(eo2);
							long int ntest = count_unit_powers(mtest);
							test_convert(mtest, to_unit, ntest, b_neg, eo2);
							if(n > 0 && ntest + (!prio_power) >= n) break;
							n = ntest;
							if(exp == 1) exp = -1;
							else exp--;
							mstruct_new = mtest;
						}
						if(exp == 1) {
							mtest = mstruct_new;
							while(exp < 10 && n > 0) {
								mtest.divide_nocopy(new MathStructure(to_unit, NULL));
								mtest.eval(eo2);
								long int ntest = count_unit_powers(mtest);
								test_convert(mtest, to_unit, ntest, b_pos, eo2);
								if(n > 0 && ntest + (!prio_power) >= n) break;
								n = ntest;
								exp++;
								mstruct_new = mtest;
							}
						}
					} else {
						exp = 1;
					}
				}
				if(cu) {
					MathStructure *mstruct_cu = new MathStructure(cu->generateMathStructure(false, eo.keep_prefixes));
					Prefix *p = NULL;
					size_t i = 1;
					Unit *u = cu->get(i, NULL, &p);
					while(u) {
						size_t i2 = i + 1;
						if(b_eval) {
							Unit *u2 = cu->get(i2);
							while(u2) {
								if(u2->baseUnit() == u->baseUnit()) {
									b_eval = false;
									break;
								}
								i2++;
								u2 = cu->get(i2);
							}
						}
						mstruct_new.setPrefixForUnit(u, p);
						i++;
						u = cu->get(i, NULL, &p);
					}
					if(exp != 1) {
						if(mstruct_cu->isPower()) (*mstruct_cu)[1].number() *= exp;
						else mstruct_cu->raise(exp);
					}
					mstruct_new.multiply_nocopy(mstruct_cu);
				} else {
					mstruct_new.multiply_nocopy(new MathStructure(to_unit, eo.keep_prefixes ? decimal_null_prefix : NULL));
					if(exp != 1) mstruct_new.last().raise(exp);
				}
				eo2.mixed_units_conversion = eo.mixed_units_conversion;
			}
			eo2.sync_units = false;
			eo2.keep_prefixes = true;
			if(b_eval) mstruct_new.eval(eo2);

			cleanMessages(mstruct, n_messages + 1);

			if(convert_to_mixed_units && eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) {
				eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP;
				return convertToMixedUnits(mstruct_new, eo2);
			} else {
				return mstruct_new;
			}
		}
	}

	return mstruct;

}
MathStructure Calculator::convertToBaseUnits(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
	MathStructure mstruct_new(mstruct);
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY || mstruct.function()->id() == FUNCTION_ID_INTERVAL)) {
		EvaluationOptions eo2 = eo;
		if(eo.interval_calculation != INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
		for(size_t i = 0; i < mstruct_new.size(); i++) {
			mstruct_new[i] = convertToBaseUnits(mstruct[i], eo2);
		}
		return mstruct_new;
	}
	size_t n_messages = messages.size();
	mstruct_new.convertToBaseUnits(true, NULL, true, eo);
	if(!mstruct_new.equals(mstruct, true, true)) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = eo.approximation;
		eo2.keep_prefixes = false;
		eo2.isolate_x = false;
		eo2.test_comparisons = false;
		//eo2.calculate_functions = false;
		mstruct_new.eval(eo2);
		cleanMessages(mstruct, n_messages + 1);
	}
	if(mstruct_new.contains(getRadUnit(), false, false, false)) {
		Unit *u = getActiveUnit("m");
		if(u) {
			MathStructure m_p_m(u);
			m_p_m.divide(u);
			mstruct_new.replace(getRadUnit(), m_p_m, false, true);
		}
	}
	return mstruct_new;
}
Unit *Calculator::findMatchingUnit(const MathStructure &mstruct) {
	switch(mstruct.type()) {
		case STRUCT_POWER: {
			if(mstruct.base()->isUnit() && mstruct.exponent()->isNumber() && mstruct.exponent()->number().isInteger() && mstruct.exponent()->number() < 10 && mstruct.exponent()->number() > -10) {
				Unit *u_base = mstruct.base()->unit();
				int exp = mstruct.exponent()->number().intValue(), exp2 = 1;
				if(u_base->subtype() == SUBTYPE_ALIAS_UNIT) {
					u_base = u_base->baseUnit();
					exp *= ((AliasUnit*) u_base)->baseExponent();
				}
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
						return u;
					} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && ((CompositeUnit*) u)->countUnits() == 1 && ((CompositeUnit*) u)->get(1, &exp2)->baseUnit() == u_base && exp == ((CompositeUnit*) u)->get(1)->baseExponent(exp2)) {
						return u;
					}
				}
				CompositeUnit *cu = new CompositeUnit("", "temporary_find_matching_unit");
				cu->add(u_base, exp);
				Unit *u = getOptimalUnit(cu);
				if(u != cu && !u->isRegistered()) {
					delete u;
				} else if(u != cu) {
					MathStructure mtest(mstruct);
					mtest.divide(u);
					mtest.eval();
					if(mtest.isNumber()) {
						delete cu;
						return u;
					}
				}
				delete cu;
			}
			return findMatchingUnit(mstruct[0]);
		}
		case STRUCT_UNIT: {
			return mstruct.unit();
		}
		case STRUCT_MULTIPLICATION: {
			if(mstruct.size() == 2 && !mstruct[0].isUnit_exp()) {
				return findMatchingUnit(mstruct[1]);
			}
			CompositeUnit *cu = new CompositeUnit("", "temporary_find_matching_unit");
			for(size_t i = 1; i <= mstruct.countChildren(); i++) {
				if(mstruct.getChild(i)->isUnit()) {
					cu->add(mstruct.getChild(i)->unit()->baseUnit(), mstruct.getChild(i)->unit()->baseExponent());
				} else if(mstruct.getChild(i)->isPower() && mstruct.getChild(i)->base()->isUnit() && mstruct.getChild(i)->exponent()->isNumber() && mstruct.getChild(i)->exponent()->number().isInteger()) {
					cu->add(mstruct.getChild(i)->base()->unit()->baseUnit(), mstruct.getChild(i)->exponent()->number().intValue());
				}
			}
			if(cu->countUnits() == 1) {
				int exp = 1, exp2 = 1;
				Unit *u_base = cu->get(1, &exp);
				if(exp == 1) return u_base;
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
						return u;
					} else if(
						u->subtype() == SUBTYPE_COMPOSITE_UNIT && ((CompositeUnit*) u)->countUnits() == 1 && ((CompositeUnit*) u)->get(1, &exp2)->baseUnit() == u_base && exp == ((CompositeUnit*) u)->get(1)->baseExponent(exp2)) {
						return u;
					}
				}
			}
			if(cu->countUnits() > 1) {
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						if(((CompositeUnit*) u)->countUnits() == cu->countUnits()) {
							bool b = true;
							for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
								int exp1 = 1, exp2 = 1;
								Unit *ui1 = cu->get(i2, &exp1);
								b = false;
								for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
									Unit *ui2 = ((CompositeUnit*) u)->get(i3, &exp2);
									if(ui1 == ui2->baseUnit()) {
										b = (exp1 == ui2->baseExponent(exp2));
										break;
									}
								}
								if(!b) break;
							}
							if(b) {
								delete cu;
								return u;
							}
						}
					}
				}
			}
			Unit *u = getOptimalUnit(cu);
			if(u != cu && !u->isRegistered()) {
				if(cu->countUnits() > 1 && u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					MathStructure m_u = ((CompositeUnit*) u)->generateMathStructure();
					if(m_u != cu->generateMathStructure()) {
						Unit *u2 = findMatchingUnit(m_u);
						if(u2) {
							MathStructure mtest(mstruct);
							mtest.divide(u2);
							mtest.eval();
							if(mtest.isNumber()) {
								delete cu;
								delete u;
								return u2;
							}
						}
					}
				}
				delete u;
			} else if(u != cu) {
				MathStructure mtest(mstruct);
				mtest.divide(u);
				mtest.eval();
				if(mtest.isNumber()) {
					delete cu;
					return u;
				}
			}
			delete cu;
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(aborted()) return NULL;
				if(!mstruct.isFunction() || !mstruct.function()->getArgumentDefinition(i + 1) || mstruct.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					Unit *u = findMatchingUnit(mstruct[i]);
					if(u) return u;
				}
			}
			break;
		}
	}
	return NULL;
}
Unit *Calculator::getBestUnit(Unit *u, bool allow_only_div, bool convert_to_local_currency) {return getOptimalUnit(u, allow_only_div, convert_to_local_currency);}
Unit *Calculator::getOptimalUnit(Unit *u, bool allow_only_div, bool convert_to_local_currency) {
	switch(u->subtype()) {
		case SUBTYPE_BASE_UNIT: {
			if(convert_to_local_currency && u->isCurrency()) {
				Unit *u_local_currency = getLocalCurrency();
				if(u_local_currency) return u_local_currency;
			}
			return u;
		}
		case SUBTYPE_ALIAS_UNIT: {
			AliasUnit *au = (AliasUnit*) u;
			if(au->baseExponent() == 1 && au->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
				if(au->isCurrency()) {
					if(!convert_to_local_currency) return u;
					Unit *u_local_currency = getLocalCurrency();
					if(u_local_currency) return u_local_currency;
				}
				return (Unit*) au->baseUnit();
			} else if(au->isSIUnit() && (au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT || au->firstBaseExponent() != 1)) {
				return u;
			} else {
				return getOptimalUnit((Unit*) au->firstBaseUnit());
			}
		}
		case SUBTYPE_COMPOSITE_UNIT: {
			CompositeUnit *cu = (CompositeUnit*) u;
			int exp, b_exp;
			int points = 0;
			bool minus = false;
			bool has_positive = false;
			int new_points;
			int new_points_m;
			int max_points = 0;
			for(size_t i = 1; i <= cu->countUnits(); i++) {
				cu->get(i, &exp);
				if(exp < 0) {
					max_points -= exp;
				} else {
					max_points += exp;
					has_positive = true;
				}
			}
			Unit *best_u = NULL;
			if(cu->countUnits() > 1) {
				for(size_t i = 0; i < units.size(); i++) {
					if(units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT && !units[i]->isHidden() && units[i]->isSIUnit()) {
						CompositeUnit *cu2 = (CompositeUnit*) units[i];
						if(cu == cu2 && !cu2->isHidden()) {
							points = max_points - 1;
						} else if(!cu2->isHidden() && cu2->isSIUnit() && cu2->countUnits() == cu->countUnits()) {
							bool b_match = true;
							for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
								int exp2;
								if(cu->get(i2, &exp) != cu2->get(i2, &exp2) || exp != exp2) {
									b_match = false;
									break;
								}
							}
							if(b_match) {
								points = max_points - 1;
								break;
							}
						}
					}
				}
				for(size_t i = 0; i < units.size() && !best_u; i++) {
					if(units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT && !units[i]->isHidden() && units[i]->isSIUnit()) {
						CompositeUnit *cu2 = (CompositeUnit*) units[i];
						if(cu2->countUnits() <= cu->countUnits()) {
							for(size_t i2 = 1; i2 <= cu2->countUnits(); i2++) {
								if(cu2->get(i2)->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT || cu2->get(i2)->baseExponent() != 1) {
									MathStructure *cu_mstruct = NULL;
									unordered_map<Unit*, MathStructure*>::iterator it = priv->composite_unit_base.find(cu2);
									if(!cu2->hasChanged() && it != priv->composite_unit_base.end()) {
										cu_mstruct = it->second;
									} else {
										if(it != priv->composite_unit_base.end()) it->second->unref();
										cu_mstruct = new MathStructure(cu2->generateMathStructure());
										if(cu_mstruct->convertToBaseUnits(true, NULL, true)) {
											beginTemporaryStopMessages();
											cu_mstruct->eval();
											endTemporaryStopMessages();
										}
										fix_to_struct2(*cu_mstruct);
										cu_mstruct->sort();
										priv->composite_unit_base[cu2] = cu_mstruct;
									}
									if(cu_mstruct->size() == cu->countUnits()) {
										bool b_match = true;
										for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
											const MathStructure *i3_m = cu_mstruct->getChild(i3);
											bool b_exp = i3_m->isPower() && i3_m->getChild(1)->isUnit();
											if((!b_exp && (!i3_m->isUnit() || cu->get(i3, &exp) != i3_m->unit() || exp != 1)) || (b_exp && (cu->get(i3, &exp) != i3_m->getChild(1)->unit() || exp == 1 || !i3_m->getChild(2)->equals(exp)))) {
												b_match = false;
												break;
											}
										}
										if(b_match) {
											points = max_points - 1;
											best_u = cu2;
										}
									}
									break;
								}
							}
						}
					}
				}
				int exp2 = 1;
				// Fix units for G
				if(cu->countUnits() == 3 && cu->get(1, &exp2)->referenceName() == "m" && exp2 == 3 && cu->get(2, &exp2)->referenceName() == "g" && exp2 == -1 && cu->get(3, &exp2)->referenceName() == "s" && exp2 == -2) {
					points = max_points - 1;
				}
			}
			Unit *bu, *u2;
			AliasUnit *au;
			for(size_t i = 0; i < units.size(); i++) {
				u2 = units[i];
				if(u2->subtype() == SUBTYPE_BASE_UNIT && (points == 0 || (points == 1 && minus))) {
					for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
						if(cu->get(i2, &exp)->baseUnit() == u2 && !cu->get(i2)->hasNonlinearRelationTo(u2)) {
							points = 1;
							best_u = u2;
							minus = !has_positive && (exp < 0);
							break;
						}
					}
				} else if(!u2->isSIUnit()) {
				} else if(u2->subtype() == SUBTYPE_ALIAS_UNIT) {
					au = (AliasUnit*) u2;
					bu = (Unit*) au->baseUnit();
					b_exp = au->baseExponent();
					new_points = 0;
					new_points_m = 0;
					if((b_exp != 1 || bu->subtype() == SUBTYPE_COMPOSITE_UNIT) && !au->hasNonlinearRelationTo(bu)) {
						if(bu->subtype() == SUBTYPE_BASE_UNIT) {
							for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
								if(cu->get(i2, &exp) == bu) {
									bool m = false;
									if(b_exp < 0 && exp < 0) {
										b_exp = -b_exp;
										exp = -exp;
									} else if(b_exp < 0) {
										b_exp = -b_exp;
										m = true;
									} else if(exp < 0) {
										exp = -exp;
										m = true;
									}
									new_points = exp - b_exp;
									if(new_points < 0) {
										new_points = -new_points;
									}
									new_points = exp - new_points;
									if(!allow_only_div && m && new_points >= max_points) {
										new_points = -1;
									}
									if(new_points > points || (!m && minus && new_points == points)) {
										points = new_points;
										minus = m;
										best_u = au;
									}
									break;
								}
							}
						} else if(au->firstBaseExponent() != 1 || au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
							MathStructure *cu_mstruct = NULL;
							unordered_map<Unit*, MathStructure*>::iterator it = priv->composite_unit_base.find(bu);
							if(!bu->hasChanged() && it != priv->composite_unit_base.end()) {
								cu_mstruct = it->second;
							} else {
								if(it != priv->composite_unit_base.end()) it->second->unref();
								cu_mstruct = new MathStructure(((CompositeUnit*) bu)->generateMathStructure());
								if(b_exp != 1) {
									if(cu_mstruct->isMultiplication()) {
										for(size_t i2 = 0; i2 < cu_mstruct->size(); i2++) {
											if((*cu_mstruct)[i2].isPower()) (*cu_mstruct)[i2][1].number() *= b_exp;
											else (*cu_mstruct)[i2].raise(b_exp);
										}
									} else if(cu_mstruct->isPower()) {
										(*cu_mstruct)[1].number() *= b_exp;
									} else {
										cu_mstruct->raise(b_exp);
									}
								}
								if(cu_mstruct->convertToBaseUnits(true, NULL, true)) {
									beginTemporaryStopMessages();
									cu_mstruct->eval();
									endTemporaryStopMessages();
								}
								fix_to_struct2(*cu_mstruct);
								priv->composite_unit_base[bu] = cu_mstruct;
							}
							if(cu_mstruct->isMultiplication()) {
								for(size_t i2 = 1; i2 <= cu_mstruct->countChildren(); i2++) {
									bu = NULL;
									if(cu_mstruct->getChild(i2)->isUnit()) {
										bu = cu_mstruct->getChild(i2)->unit();
										b_exp = 1;
									} else if(cu_mstruct->getChild(i2)->isPower() && cu_mstruct->getChild(i2)->base()->isUnit() && cu_mstruct->getChild(i2)->exponent()->isNumber() && cu_mstruct->getChild(i2)->exponent()->number().isInteger()) {
										bu = cu_mstruct->getChild(i2)->base()->unit();
										b_exp = cu_mstruct->getChild(i2)->exponent()->number().intValue();
									}
									if(bu) {
										bool b = false;
										for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
											if(cu->get(i3, &exp) == bu) {
												b = true;
												bool m = false;
												if(exp < 0 && b_exp > 0) {
													new_points -= b_exp;
													exp = -exp;
													m = true;
												} else if(exp > 0 && b_exp < 0) {
													new_points += b_exp;
													b_exp = -b_exp;
													m = true;
												} else {
													if(b_exp < 0) new_points_m += b_exp;
													else new_points_m -= b_exp;
												}
												if(exp < 0) {
													exp = -exp;
													b_exp = -b_exp;
												}
												if(exp >= b_exp) {
													if(m) new_points_m += exp - (exp - b_exp);
													else new_points += exp - (exp - b_exp);
												} else {
													if(m) new_points_m += exp - (b_exp - exp);
													else new_points += exp - (b_exp - exp);
												}
												break;
											}
										}
										if(!b) {
											if(b_exp < 0) b_exp = -b_exp;
											new_points -= b_exp;
											new_points_m -= b_exp;
										}
									}
								}
								if(!allow_only_div && new_points_m >= max_points) {
									new_points_m = -1;
								}
								if(new_points > points && new_points >= new_points_m) {
									minus = false;
									points = new_points;
									best_u = au;
								} else if(new_points_m > points || (new_points_m == points && minus)) {
									minus = true;
									points = new_points_m;
									best_u = au;
								}
							}
						}
					}
				}
				if(points >= max_points && !minus) break;
			}
			if(!best_u) return u;
			best_u = getOptimalUnit(best_u, false, convert_to_local_currency);
			if(points > 1 && points < max_points - 1) {
				CompositeUnit *cu_new = new CompositeUnit("", "temporary_composite_convert");
				bool return_cu = minus;
				if(minus) {
					cu_new->add(best_u, -1);
				} else {
					cu_new->add(best_u);
				}
				MathStructure cu_mstruct = ((CompositeUnit*) u)->generateMathStructure();
				if(minus) cu_mstruct *= best_u;
				else cu_mstruct /= best_u;
				if(cu_mstruct.convertToBaseUnits(true, NULL, true)) {
					beginTemporaryStopMessages();
					cu_mstruct.eval();
					endTemporaryStopMessages();
				}
				CompositeUnit *cu2 = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
				bool b = false;
				for(size_t i = 1; i <= cu_mstruct.countChildren(); i++) {
					if(cu_mstruct.getChild(i)->isUnit()) {
						b = true;
						cu2->add(cu_mstruct.getChild(i)->unit());
					} else if(cu_mstruct.getChild(i)->isPower() && cu_mstruct.getChild(i)->base()->isUnit() && cu_mstruct.getChild(i)->exponent()->isNumber() && cu_mstruct.getChild(i)->exponent()->number().isInteger()) {
						if(cu_mstruct.getChild(i)->exponent()->number().isGreaterThan(10) || cu_mstruct.getChild(i)->exponent()->number().isLessThan(-10)) {
							if(aborted() || cu_mstruct.getChild(i)->exponent()->number().isGreaterThan(1000) || cu_mstruct.getChild(i)->exponent()->number().isLessThan(-1000)) {
								b = false;
								break;
							}
						}
						b = true;
						cu2->add(cu_mstruct.getChild(i)->base()->unit(), cu_mstruct.getChild(i)->exponent()->number().intValue());
					}
				}
				if(b) {
					Unit *u2 = getOptimalUnit(cu2, true, convert_to_local_currency);
					b = false;
					if(u2->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						for(size_t i3 = 1; i3 <= ((CompositeUnit*) u2)->countUnits(); i3++) {
							Unit *cu_unit = ((CompositeUnit*) u2)->get(i3, &exp);
							for(size_t i4 = 1; i4 <= cu_new->countUnits(); i4++) {
								if(cu_new->get(i4, &b_exp) == cu_unit) {
									b = true;
									cu_new->setExponent(i4, b_exp + exp);
									break;
								}
							}
							if(!b) cu_new->add(cu_unit, exp);
						}
						return_cu = true;
					} else if(u2->subtype() == SUBTYPE_ALIAS_UNIT) {
						return_cu = true;
						for(size_t i3 = 1; i3 <= cu_new->countUnits(); i3++) {
							if(cu_new->get(i3, &exp) == u2) {
								b = true;
								cu_new->setExponent(i3, exp + 1);
								break;
							}
						}
						if(!b) cu_new->add(u2);
					}
					if(!u2->isRegistered() && u2 != cu2) delete u2;
				}
				delete cu2;
				if(return_cu) {
					return cu_new;
				} else {
					delete cu_new;
					return best_u;
				}
			}
			if(minus) {
				CompositeUnit *cu_new = new CompositeUnit("", "temporary_composite_convert");
				cu_new->add(best_u, -1);
				return cu_new;
			} else {
				return best_u;
			}
		}
	}
	return u;
}

#define CONVERT_MULTISUB(m) \
	bool child_updated = false;\
	for(size_t i = 1; i <= m.countChildren(); i++) {\
		if(m.getChild(i)->size() > 0 && !aborted() && m.getChild(i)->containsType(STRUCT_UNIT, true) && (!m.getChild(i)->isPower() || !m.getChild(i)->base()->isUnit() || !m.getChild(i)->exponent()->isNumber() || !m.getChild(i)->exponent()->number().isRational())) {\
			m[i - 1] = convertToOptimalUnit(m[i - 1], eo, convert_to_si_units);\
			if(!m[i - 1].equals(mstruct[i - 1], true, true)) child_updated = true;\
		}\
	}\
	if(child_updated) m.eval(eo2);

#define CONVERT_MULTISUB_N(m) \
	bool child_updated = false;\
	for(size_t i = 1; i <= m.countChildren(); i++) {\
		if(m.getChild(i)->size() > 0 && !aborted() && m.getChild(i)->containsType(STRUCT_UNIT, true) && (!m.getChild(i)->isPower() || !m.getChild(i)->base()->isUnit() || !m.getChild(i)->exponent()->isNumber() || !m.getChild(i)->exponent()->number().isRational())) {\
			MathStructure m_i_old(m[i - 1]);\
			eo2.calculate_functions = false;\
			m[i - 1].eval(eo2);\
			eo2.calculate_functions = eo.calculate_functions;\
			m[i - 1] = convertToOptimalUnit(m[i - 1], eo, convert_to_si_units);\
			m.childUpdated(i);\
			if(!m[i - 1].equals(m_i_old, true, true)) child_updated = true;\
		}\
	}\
	if(child_updated) m.eval(eo2);

bool replace_prefixes(MathStructure &m, const EvaluationOptions &eo) {
	if(m.isUnit() && m.prefix()) {
		if(m.prefix() == CALCULATOR->getDecimalNullPrefix() || m.prefix() == CALCULATOR->getBinaryNullPrefix()) {
			m.unformat(eo);
			return false;
		} else {
			m.unformat(eo);
			return true;
		}
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_prefixes(m[i], eo)) b = true;
	}
	if(b && (m.isMultiplication() || m.isPower())) m.calculatesub(eo, eo, false);
	return b;
}

#define UNFORMAT(m) \
	if(eo2.keep_prefixes) {\
		m.unformat(eo2);\
	} else {\
		eo2.keep_prefixes = true;\
		m.unformat(eo2);\
		eo2.keep_prefixes = false;\
		replace_prefixes(m, eo2);\
	}

MathStructure Calculator::convertToBestUnit(const MathStructure &mstruct, const EvaluationOptions &eo, bool convert_to_si_units) {return convertToOptimalUnit(mstruct, eo, convert_to_si_units);}
MathStructure Calculator::convertToOptimalUnit(const MathStructure &mstruct, const EvaluationOptions &eo, bool convert_to_si_units) {
	EvaluationOptions eo2 = eo;
	//eo2.calculate_functions = false;
	eo2.sync_units = false;
	eo2.isolate_x = false;
	eo2.test_comparisons = false;
	switch(mstruct.type()) {
		case STRUCT_POWER: {
			if(mstruct.base()->isUnit() && mstruct.exponent()->isNumber() && mstruct.exponent()->number().isRational() && !mstruct.exponent()->number().isZero()) {
				MathStructure mstruct_new(mstruct);
				int old_points = 0;
				bool overflow = false;
				if(mstruct_new.exponent()->isInteger()) old_points = mstruct_new.exponent()->number().intValue(&overflow);
				else old_points = mstruct_new.exponent()->number().numerator().intValue(&overflow) + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
				if(overflow) return mstruct_new;
				bool old_minus = false;
				if(old_points < 0) {
					old_points = -old_points;
					old_minus = true;
				}
				bool is_si_units = mstruct_new.base()->unit()->isSIUnit();
				if(mstruct_new.base()->unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					mstruct_new.convertToBaseUnits(true, NULL, true, eo2, true);
					if(mstruct_new.equals(mstruct, true, true)) {
						return mstruct_new;
					} else {
						mstruct_new.eval(eo2);
					}
					mstruct_new = convertToOptimalUnit(mstruct_new, eo, convert_to_si_units);
					if(mstruct_new.equals(mstruct, true, true)) return mstruct_new;
				} else {
					CompositeUnit *cu = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
					cu->add(mstruct_new.base()->unit(), mstruct_new.exponent()->number().numerator().intValue());
					Unit *u = getOptimalUnit(cu, false, eo.local_currency_conversion);
					if(u == cu) {
						delete cu;
						return mstruct_new;
					}
					if(eo.approximation == APPROXIMATION_EXACT && cu->hasApproximateRelationTo(u, true)) {
						if(!u->isRegistered()) delete u;
						delete cu;
						return mstruct_new;
					}
					delete cu;
					mstruct_new = convert(mstruct_new, u, eo, true);
					if(!u->isRegistered()) delete u;
					UNFORMAT(mstruct_new);
				}
				int new_points = 0;
				bool new_is_si_units = true;
				bool new_minus = true;
				bool is_currency = false;
				if(mstruct_new.isMultiplication()) {
					for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
						if(mstruct_new.getChild(i)->isUnit()) {
							if(new_is_si_units && !mstruct_new.getChild(i)->unit()->isSIUnit()) new_is_si_units = false;
							is_currency = mstruct_new.getChild(i)->unit()->isCurrency();
							new_points++;
							new_minus = false;
						} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isRational()) {
							int points = 0;
							if(mstruct_new.getChild(i)->exponent()->isInteger()) points = mstruct_new.getChild(i)->exponent()->number().intValue();
							else points = mstruct_new.getChild(i)->exponent()->number().numerator().intValue() + mstruct_new.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_new.getChild(i)->exponent()->number().isNegative() ? -1 : 1);
							if(new_is_si_units && !mstruct_new.getChild(i)->base()->unit()->isSIUnit()) new_is_si_units = false;
							is_currency = mstruct_new.getChild(i)->base()->unit()->isCurrency();
							if(points < 0) {
								new_points -= points;
							} else {
								new_points += points;
								new_minus = false;
							}

						}
					}
				} else if(mstruct_new.isPower() && mstruct_new.base()->isUnit() && mstruct_new.exponent()->isNumber() && mstruct_new.exponent()->number().isRational()) {
					int points = 0;
					if(mstruct_new.exponent()->isInteger()) points = mstruct_new.exponent()->number().intValue();
					else points = mstruct_new.exponent()->number().numerator().intValue() + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
					if(new_is_si_units && !mstruct_new.base()->unit()->isSIUnit()) new_is_si_units = false;
					is_currency = mstruct_new.base()->unit()->isCurrency();
					if(points < 0) {
						new_points = -points;
					} else {
						new_points = points;
						new_minus = false;
					}
				} else if(mstruct_new.isUnit()) {
					if(!mstruct_new.unit()->isSIUnit()) new_is_si_units = false;
					is_currency = mstruct_new.unit()->isCurrency();
					new_points = 1;
					new_minus = false;
				}
				if(new_points == 0) return mstruct;
				if((new_points > old_points && (!convert_to_si_units || is_si_units || !new_is_si_units)) || (new_points == old_points && (new_minus || !old_minus) && (!is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || !new_is_si_units))) return mstruct;
				return mstruct_new;
			}
		}
		case STRUCT_BITWISE_XOR: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_NOT: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_COMPARISON: {}
		case STRUCT_FUNCTION: {}
		case STRUCT_VECTOR: {}
		case STRUCT_ADDITION: {
			if((mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS) || !mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
			MathStructure mstruct_new(mstruct);
			bool b = false;
			if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY || mstruct.function()->id() == FUNCTION_ID_INTERVAL) && eo.interval_calculation != INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC && eo.interval_calculation != INTERVAL_CALCULATION_NONE) {
				EvaluationOptions eo3 = eo;
				eo3.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				for(size_t i = 0; i < mstruct_new.size(); i++) {
					if(aborted()) return mstruct;
					mstruct_new[i] = convertToOptimalUnit(mstruct_new[i], eo3, convert_to_si_units);
					if(!b && !mstruct_new[i].equals(mstruct[i], true, true)) b = true;
				}
			} else {
				for(size_t i = 0; i < mstruct_new.size(); i++) {
					if(aborted()) return mstruct;
					if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
						mstruct_new[i] = convertToOptimalUnit(mstruct_new[i], eo, convert_to_si_units);
						if(!b && !mstruct_new[i].equals(mstruct[i], true, true)) b = true;
					}
				}
			}
			if(b) {
				mstruct_new.childrenUpdated();
				if(mstruct.isAddition()) {
					mstruct_new.eval(eo2);
				} else if(mstruct_new.isVector() && mstruct_new.size() == 3) {
					bool b_torque = true;
					Unit *u_joule = NULL;
					for(size_t i = 0; i < 3 && b_torque; i++) {
						if(mstruct_new[i].isMultiplication()) {
							b_torque = false;
							for(size_t i2 = 0; i2 < mstruct_new[i].size(); i2++) {
								if(!b_torque && mstruct_new[i][i2].isUnit() && ((u_joule && mstruct_new[i][i2].unit() == u_joule) || (!u_joule && mstruct_new[i][i2].unit()->referenceName() == "J"))) {
									b_torque = true;
									if(!u_joule) u_joule = mstruct_new[i][i2].unit();
								} else if(mstruct_new[i][i2].containsType(STRUCT_UNIT, true)) {
									b_torque = false;
									break;
								}
							}
						} else if(mstruct_new[i].isUnit() && ((u_joule && mstruct_new[i].unit() == u_joule) || (!u_joule && mstruct_new[i].unit()->referenceName() == "J"))) {
							if(!u_joule) u_joule = mstruct_new[i].unit();
						} else {
							b_torque = false;
						}
					}
					if(b_torque && u_joule) {
						mstruct_new = convert(mstruct_new, u_joule->baseUnit(), eo);
					}
				}
			}
			return mstruct_new;
		}
		case STRUCT_UNIT: {
			if((!mstruct.unit()->isCurrency() || !eo.local_currency_conversion) && (!convert_to_si_units || mstruct.unit()->isSIUnit())) return mstruct;
			Unit *u = getOptimalUnit(mstruct.unit(), false, eo.local_currency_conversion);
			if(u != mstruct.unit()) {
				if((u->isSIUnit() || (u->isCurrency() && eo.local_currency_conversion)) && (eo.approximation != APPROXIMATION_EXACT || !mstruct.unit()->hasApproximateRelationTo(u, true))) {
					MathStructure mstruct_new = convert(mstruct, u, eo, true);
					if(!u->isRegistered()) delete u;
					UNFORMAT(mstruct_new)
					return mstruct_new;
				}
				if(!u->isRegistered()) delete u;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
			int old_points = 0;
			bool old_minus = true;
			bool is_si_units = true;
			bool is_currency = false;
			MathStructure mstruct_old(mstruct);
			for(size_t i = 1; i <= mstruct_old.countChildren(); i++) {
				if(aborted()) return mstruct_old;
				if(mstruct_old.getChild(i)->isUnit()) {
					if(is_si_units && !mstruct_old.getChild(i)->unit()->isSIUnit()) is_si_units = false;
					is_currency = mstruct_old.getChild(i)->unit()->isCurrency();
					old_points++;
					old_minus = false;
				} else if(mstruct_old.getChild(i)->isPower() && mstruct_old.getChild(i)->base()->isUnit() && mstruct_old.getChild(i)->exponent()->isNumber() && mstruct_old.getChild(i)->exponent()->number().isRational()) {
					int points = 0;
					if(mstruct_old.getChild(i)->exponent()->number().isInteger()) points = mstruct_old.getChild(i)->exponent()->number().intValue();
					else points = mstruct_old.getChild(i)->exponent()->number().numerator().intValue() + mstruct_old.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_old.getChild(i)->exponent()->number().isNegative() ? -1 : 1);;
					if(is_si_units && !mstruct_old.getChild(i)->base()->unit()->isSIUnit()) is_si_units = false;
						is_currency = mstruct_old.getChild(i)->base()->unit()->isCurrency();
					if(points < 0) {
						old_points -= points;
					} else {
						old_points += points;
						old_minus = false;
					}
				}
			}
			if((!is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || is_si_units) && old_points <= 1 && !old_minus) {
				CONVERT_MULTISUB(mstruct_old)
				return mstruct_old;
			}
			MathStructure mstruct_new(mstruct_old);
			mstruct_new.convertToBaseUnits(true, NULL, true, eo2, true);
			if(!mstruct_new.equals(mstruct, true, true)) {
				mstruct_new.eval(eo2);
			}
			if(mstruct_new.type() != STRUCT_MULTIPLICATION) {
				if(!mstruct_new.containsInterval(true, true, false, 1, true) && !aborted()) mstruct_new = convertToOptimalUnit(mstruct_new, eo, convert_to_si_units);
			} else {
				CompositeUnit *cu = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
				bool b = false;
				mstruct_new.sort();
				for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
					if(aborted()) {
						delete cu;
						CONVERT_MULTISUB(mstruct_old)
						return mstruct_old;
					}
					if(mstruct_new.getChild(i)->isUnit()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->unit());
					} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isInteger()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->base()->unit(), mstruct_new.getChild(i)->exponent()->number().intValue());
					}
				}
				bool is_converted = false;
				if(b) {
					Unit *u = getOptimalUnit(cu, false, eo.local_currency_conversion);
					if(u != cu) {
						if(eo.approximation != APPROXIMATION_EXACT || !cu->hasApproximateRelationTo(u, true)) {
							mstruct_new = convert(mstruct_new, u, eo, true);
							UNFORMAT(mstruct_new)
							is_converted = true;
						}
						if(!u->isRegistered()) delete u;
					}
				}
				delete cu;
				if((!b || !is_converted) && (!convert_to_si_units || is_si_units)) {
					CONVERT_MULTISUB(mstruct_old)
					return mstruct_old;
				}
			}
			if(((eo.approximation == APPROXIMATION_EXACT && !mstruct_old.isApproximate()) && (mstruct_new.isApproximate() || (mstruct_old.containsInterval(true, true, false, 0, true) <= 0 && mstruct_new.containsInterval(true, true, false, 0, true) > 0))) || mstruct_new.equals(mstruct_old, true, true)) {
				CONVERT_MULTISUB(mstruct_old)
				return mstruct_old;
			}
			int new_points = 0;
			bool new_minus = true;
			bool new_is_si_units = true;
			bool new_is_currency = false;
			if(mstruct_new.isMultiplication()) {
				for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
					if(aborted()) return mstruct_old;
					if(mstruct_new.getChild(i)->isUnit()) {
						if(new_is_si_units && !mstruct_new.getChild(i)->unit()->isSIUnit()) new_is_si_units = false;
						new_is_currency = mstruct_new.getChild(i)->unit()->isCurrency();
						new_points++;
						new_minus = false;
					} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isRational()) {
						int points = 0;
						if(mstruct_new.getChild(i)->exponent()->number().isInteger()) points = mstruct_new.getChild(i)->exponent()->number().intValue();
						else points = mstruct_new.getChild(i)->exponent()->number().numerator().intValue() + mstruct_new.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_new.getChild(i)->exponent()->number().isNegative() ? -1 : 1);
						if(new_is_si_units && !mstruct_new.getChild(i)->base()->unit()->isSIUnit()) new_is_si_units = false;
						new_is_currency = mstruct_new.getChild(i)->base()->unit()->isCurrency();
						if(points < 0) {
							new_points -= points;
						} else {
							new_points += points;
							new_minus = false;
						}
					}
				}
			} else if(mstruct_new.isPower() && mstruct_new.base()->isUnit() && mstruct_new.exponent()->isNumber() && mstruct_new.exponent()->number().isRational()) {
				int points = 0;
				if(mstruct_new.exponent()->number().isInteger()) points = mstruct_new.exponent()->number().intValue();
				else points = mstruct_new.exponent()->number().numerator().intValue() + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
				if(new_is_si_units && !mstruct_new.base()->unit()->isSIUnit()) new_is_si_units = false;
				new_is_currency = mstruct_new.base()->unit()->isCurrency();
				if(points < 0) {
					new_points = -points;
				} else {
					new_points = points;
					new_minus = false;
				}
			} else if(mstruct_new.isUnit()) {
				if(!mstruct_new.unit()->isSIUnit()) new_is_si_units = false;
				new_is_currency = mstruct_new.unit()->isCurrency();
				new_points = 1;
				new_minus = false;
			}
			if(new_points == 0 || (new_points > old_points && (!convert_to_si_units || is_si_units || !new_is_si_units)) || (new_points == old_points && (new_minus || !old_minus) && (!new_is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || !new_is_si_units))) {
				CONVERT_MULTISUB(mstruct_old)
				return mstruct_old;
			}
			CONVERT_MULTISUB_N(mstruct_new)
			return mstruct_new;
		}
		default: {}
	}
	return mstruct;
}
MathStructure Calculator::convertToCompositeUnit(const MathStructure &mstruct, CompositeUnit *cu, const EvaluationOptions &eo, bool always_convert) {
	return convert(mstruct, cu, eo, always_convert);
}
MathStructure Calculator::convert(const MathStructure &mstruct_to_convert, string str2, const EvaluationOptions &eo, MathStructure *to_struct) {
	return convert(mstruct_to_convert, str2, eo, to_struct, false, NULL);
}

void set_null_prefixes(MathStructure &m) {
	if(!m.isUnit() || !m.prefix()) m.setPrefix(CALCULATOR->decimal_null_prefix);
	for(size_t i = 0; i < m.size(); i++) {
		set_null_prefixes(m[i]);
	}
}

MathStructure Calculator::convert(const MathStructure &mstruct_to_convert, string str2, const EvaluationOptions &eo, MathStructure *to_struct, bool transform_orig, MathStructure *parsed_struct) {
	if(to_struct) to_struct->setUndefined();
	remove_blank_ends(str2);
	if(str2.empty()) return mstruct_to_convert;
	current_stage = MESSAGE_STAGE_CONVERSION;
	int do_prefix = 0;
	if(str2.length() > 1 && str2[1] == '?' && (str2[0] == 'b' || str2[0] == 'a' || str2[0] == 'd')) {
		do_prefix = 2;
	} else if(str2[0] == '?') {
		do_prefix = 1;
	}
	EvaluationOptions eo2 = eo;
	eo2.keep_prefixes = !do_prefix;
	if(str2.find(SIGN_MINUS) == 0) str2.replace(0, strlen(SIGN_MINUS), "-");
	if(str2[0] == '-') eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	else if(str2[0] == '+') eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
	else if(eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP;
	if(do_prefix || str2[0] == '0' || str2[0] == '+' || str2[0] == '-') {
		str2 = str2.substr(do_prefix > 1 ? 2 : 1, str2.length() - (do_prefix > 1 ? 2 : 1));
		remove_blank_ends(str2);
		if(str2.empty()) {
			current_stage = MESSAGE_STAGE_UNSET;
			return convertToMixedUnits(mstruct_to_convert, eo2);
		}
	}
	MathStructure mstruct;
	bool b = false;
	Unit *u = getActiveUnit(str2);
	if(!u) u = getCompositeUnit(str2);
	Variable *v = NULL;
	if(!u) v = getActiveVariable(str2);
	if(!u && !v) {
		for(size_t i = 0; i < signs.size(); i++) {
			if(str2 == signs[i]) {
				u = getUnit(real_signs[i]);
				if(!u) v = getActiveVariable(real_signs[i]);
				break;
			}
		}
	}
	if(v && !v->isKnown()) v = NULL;
	if(v) {
		u = CALCULATOR->getActiveUnit(v->referenceName() + "_unit");
		if(!u) {
			if(v->referenceName() == "bohr_radius") u = CALCULATOR->getActiveUnit("bohr_unit");
			else if(v->referenceName() == "elementary_charge") u = CALCULATOR->getActiveUnit("e_unit");
			else if(v->referenceName() == "electron_mass") u = CALCULATOR->getActiveUnit("electron_unit");
		}
		if(u) v = NULL;
	}
	if(u) {
		if(to_struct) to_struct->set(u);
		mstruct.set(convert(mstruct_to_convert, u, eo2, false, false, transform_orig, parsed_struct));
		b = true;
	} else if(v) {
		if(to_struct) to_struct->set(v);
		mstruct.set(convert(mstruct_to_convert, (KnownVariable*) v, eo2));
		b = true;
	} else {
		current_stage = MESSAGE_STAGE_CONVERSION_PARSING;
		beginTemporaryStopMessages();
		CompositeUnit cu("", eo.parse_options.limit_implicit_multiplication ? "01" : "00", "", str2);
		MathStructure munits;
		if(stopped_errors_count[disable_errors_ref - 1] > 0) {
			endTemporaryStopMessages();
			ParseOptions po = eo.parse_options;
			po.units_enabled = true;
			parse(&munits, str2, po);
			eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
		} else {
			munits.setUndefined();
			endTemporaryStopMessages(true);
		}
		if(cu.countUnits() == 2 && cu.get(1)->referenceName() == "g" && cu.get(2)->referenceName() == "m" && str2.substr(0, 2) == "kg") {
			int exp = 1;
			Prefix *p = NULL;
			if(cu.get(1, &exp, &p) && exp == 1 && p && p->value() == 1000 && cu.get(2, &exp, &p) && exp == -2) {
				Unit *u = getUnit("pond");
				if(u) {
					MathStructure mtest(convertToBaseUnits(mstruct_to_convert, eo));
					mtest.sort();
					if(mtest.isMultiplication() && mtest.size() >= 3 && mtest[mtest.size() - 3].isUnit() && mtest[mtest.size() - 3].unit()->referenceName() == "g" && mtest[mtest.size() - 2].isPower() && mtest[mtest.size() - 2][1].isMinusOne() && mtest[mtest.size() - 2][0].isUnit() && mtest[mtest.size() - 2][0].unit()->referenceName() == "m" && mtest[mtest.size() - 1].isPower() && mtest[mtest.size() - 1][1] == Number(-2, 1) && mtest[mtest.size() - 1][0].isUnit() && mtest[mtest.size() - 1][0].unit()->referenceName() == "s") {
						str2.replace(1, 2, "pond");
						cu.setBaseExpression(str2);
					}
				}
			}
		}
		current_stage = MESSAGE_STAGE_CONVERSION;
		if(to_struct) {
			if(!munits.isUndefined()) to_struct->set(munits);
			else to_struct->set(cu.generateMathStructure(true));
		}
		if(cu.countUnits() > 0) {
			if(cu.countUnits() == 1) {
				Prefix *p = NULL; int exp = 0;
				u = cu.get(1, &exp, &p);
				if(!u || exp != 1 || p) u = &cu;
			} else {
				u = &cu;
			}
			mstruct.set(convert(mstruct_to_convert, u, eo2, false, false, transform_orig, parsed_struct));
			b = true;
		} else {
			mstruct = mstruct_to_convert;
		}
		if(!munits.isUndefined()) {
			mstruct.divide(munits);
			eo2.sync_units = true;
			eo2.keep_prefixes = false;
			mstruct.eval(eo2);
			if((eo.auto_post_conversion == POST_CONVERSION_OPTIMAL || eo.auto_post_conversion == POST_CONVERSION_OPTIMAL_SI) && mstruct.containsType(STRUCT_UNIT, true)) {
				mstruct.set(CALCULATOR->convertToOptimalUnit(mstruct, eo, false));
			}
			set_null_prefixes(munits);
			mstruct *= munits;
			b = true;
		}
	}
	if(!b) return mstruct_to_convert;
	if(!v && eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo2));
	current_stage = MESSAGE_STAGE_UNSET;
	return mstruct;
}

