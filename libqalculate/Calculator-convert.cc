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
		while(eo.mixed_units_conversion > MIXED_UNITS_CONVERSION_DOWNWARDS && nr.isGreaterThan(nr_one)) {
			Unit *best_u = NULL;
			Number best_nr;
			int best_priority = 0;
			for(size_t i = 0; i < units.size(); i++) {
				Unit *ui = units[i];
				if(ui->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) ui)->firstBaseUnit() == u  && ((AliasUnit*) ui)->firstBaseExponent() == 1) {
					AliasUnit *aui = (AliasUnit*) ui;
					int priority_i = aui->mixWithBase();
					if(((priority_i > 0 && (!best_u || priority_i <= best_priority)) || (best_priority == 0 && priority_i == 0 && ((eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && aui->expression().find_first_not_of(NUMBERS) == string::npos) || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL))) && (aui->mixWithBaseMinimum() <= 1 || nr.isGreaterThanOrEqualTo(aui->mixWithBaseMinimum()))) {
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
		while(u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT && ((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL || (eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos)) && !nr.isInteger() && !nr.isZero()) {
			Number int_nr(nr);
			int_nr.trunc();
			if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP && int_nr.isZero()) break;
			nr -= int_nr;
			MathStructure mstruct_nr(nr);
			MathStructure m_exp(m_one);
			((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
			mstruct_nr.eval(eo);
			while(!accept_obsolete && ((AliasUnit*) u)->firstBaseUnit()->subtype() == SUBTYPE_ALIAS_UNIT && abs(((AliasUnit*) ((AliasUnit*) u)->firstBaseUnit())->mixWithBase()) > 1) {
				u = ((AliasUnit*) u)->firstBaseUnit();
				if(((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL || (eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos))) {
					((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
					mstruct_nr.eval(eo);
					if(!mstruct_nr.isNumber() || !m_exp.isOne()) break;
				} else {
					mstruct_nr.setUndefined();
					break;
				}
			}
			if(!mstruct_nr.isNumber() || !m_exp.isOne()) break;
			if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL && mstruct_nr.number().isLessThanOrEqualTo(nr)) break;
			u = ((AliasUnit*) u)->firstBaseUnit();
			nr = mstruct_nr.number();
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
	if(b_var_units && !to_var->unit().empty() && to_var->isExpression()) {
		CompositeUnit cu("", "temporary_composite_convert", "", to_var->unit());
		if(cu.countUnits() > 0) {
			AliasUnit au("", "temporary_alias_convert", "", "", "", &cu, to_var->expression());
			bool unc_rel = false;
			if(!to_var->uncertainty(&unc_rel).empty()) au.setUncertainty(to_var->uncertainty(), unc_rel);
			au.setApproximate(to_var->isApproximate());
			au.setPrecision(to_var->precision());
			MathStructure mstruct_new(convert(mstruct, &au, eo, false, false));
			if(mstruct_new.contains(&au)) {
				mstruct_new.replace(&au, to_var);
				return mstruct_new;
			}
		}
	}
	MathStructure mstruct_new(mstruct);
	mstruct_new /= to_var->get();
	mstruct_new.eval(eo);
	mstruct_new *= to_var;
	cleanMessages(mstruct, n_messages + 1);
	return mstruct_new;
}
MathStructure Calculator::convert(const MathStructure &mstruct, Unit *to_unit, const EvaluationOptions &eo, bool always_convert, bool convert_to_mixed_units) {
	if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
	CompositeUnit *cu = NULL;
	if(to_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) cu = (CompositeUnit*) to_unit;
	if(cu && cu->countUnits() == 0) return mstruct;
	MathStructure mstruct_new(mstruct);
	size_t n_messages = messages.size();
	if(to_unit->hasNonlinearRelationTo(to_unit->baseUnit()) && to_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		mstruct_new = convert(mstruct, to_unit->baseUnit(), eo, always_convert, convert_to_mixed_units);
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

	if(!mstruct_new.isPower() && !mstruct_new.isUnit() && !mstruct_new.isMultiplication()) {
		if(mstruct_new.size() > 0) {
			for(size_t i = 0; i < mstruct_new.size(); i++) {
				if(aborted()) return mstruct;
				if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					mstruct_new[i] = convert(mstruct_new[i], to_unit, eo, false, convert_to_mixed_units);
					if(!b_changed && !mstruct_new.equals(mstruct[i], true, true)) b_changed = true;
				}
			}
			if(b_changed) {
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
		if(mstruct_new.convert(to_unit, true, NULL, false, eo2, eo.keep_prefixes ? decimal_null_prefix : NULL) || always_convert) {
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
		if(b) {
			eo2.approximation = eo.approximation;
			eo2.sync_units = true;
			eo2.keep_prefixes = false;
			MathStructure mbak(mstruct_new);
			mstruct_new.divide(MathStructure(to_unit, NULL));
			mstruct_new.eval(eo2);
			if(mstruct_new.containsType(STRUCT_UNIT)) {
				mbak.inverse();
				mbak.divide(MathStructure(to_unit, NULL));
				mbak.eval(eo2);
				if(!mbak.containsType(STRUCT_UNIT)) mstruct_new = mbak;
			}

			if(cu) {
				MathStructure mstruct_cu(cu->generateMathStructure(false, eo.keep_prefixes));
				Prefix *p = NULL;
				size_t i = 1;
				Unit *u = cu->get(i, NULL, &p);
				while(u) {
					mstruct_new.setPrefixForUnit(u, p);
					i++;
					u = cu->get(i, NULL, &p);
				}
				mstruct_new.multiply(mstruct_cu);
			} else {
				mstruct_new.multiply(MathStructure(to_unit, eo.keep_prefixes ? decimal_null_prefix : NULL));
			}

			eo2.sync_units = false;
			eo2.keep_prefixes = true;
			mstruct_new.eval(eo2);

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
	size_t n_messages = messages.size();
	MathStructure mstruct_new(mstruct);
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
	return mstruct_new;
}
Unit *Calculator::findMatchingUnit(const MathStructure &mstruct) {
	switch(mstruct.type()) {
		case STRUCT_POWER: {
			if(mstruct.base()->isUnit() && mstruct.exponent()->isNumber() && mstruct.exponent()->number().isInteger() && mstruct.exponent()->number() < 10 && mstruct.exponent()->number() > -10) {
				Unit *u_base = mstruct.base()->unit();
				int exp = mstruct.exponent()->number().intValue();
				if(u_base->subtype() == SUBTYPE_ALIAS_UNIT) {
					u_base = u_base->baseUnit();
					exp *= ((AliasUnit*) u_base)->baseExponent();
				}
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
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
					cu->add(mstruct.getChild(i)->unit()->baseUnit());
				} else if(mstruct.getChild(i)->isPower() && mstruct.getChild(i)->base()->isUnit() && mstruct.getChild(i)->exponent()->isNumber() && mstruct.getChild(i)->exponent()->number().isInteger()) {
					cu->add(mstruct.getChild(i)->base()->unit()->baseUnit(), mstruct.getChild(i)->exponent()->number().intValue());
				}
			}
			if(cu->countUnits() == 1) {
				int exp = 1;
				Unit *u_base = cu->get(1, &exp);
				if(exp == 1) return u_base;
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
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
										b = (exp1 == exp2);
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
			for(size_t i = 0; i < units.size(); i++) {
				if(units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
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
			Unit *best_u = NULL;
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
							MathStructure cu_mstruct = ((CompositeUnit*) bu)->generateMathStructure();
							if(b_exp != 1) {
								if(cu_mstruct.isMultiplication()) {
									for(size_t i2 = 0; i2 < cu_mstruct.size(); i2++) {
										if(cu_mstruct[i2].isPower()) cu_mstruct[i2][1].number() *= b_exp;
										else cu_mstruct[i2].raise(b_exp);
									}
								} else if(cu_mstruct.isPower()) {
									cu_mstruct[1].number() *= b_exp;
								} else {
									cu_mstruct.raise(b_exp);
								}
							}
							cu_mstruct = convertToBaseUnits(cu_mstruct);
							if(cu_mstruct.isMultiplication()) {
								for(size_t i2 = 1; i2 <= cu_mstruct.countChildren(); i2++) {
									bu = NULL;
									if(cu_mstruct.getChild(i2)->isUnit()) {
										bu = cu_mstruct.getChild(i2)->unit();
										b_exp = 1;
									} else if(cu_mstruct.getChild(i2)->isPower() && cu_mstruct.getChild(i2)->base()->isUnit() && cu_mstruct.getChild(i2)->exponent()->isNumber() && cu_mstruct.getChild(i2)->exponent()->number().isInteger()) {
										bu = cu_mstruct.getChild(i2)->base()->unit();
										b_exp = cu_mstruct.getChild(i2)->exponent()->number().intValue();
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
				cu_mstruct = convertToBaseUnits(cu_mstruct);
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
			if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
			MathStructure mstruct_new(mstruct);
			bool b = false;
			for(size_t i = 0; i < mstruct_new.size(); i++) {
				if(aborted()) return mstruct;
				if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					mstruct_new[i] = convertToOptimalUnit(mstruct_new[i], eo, convert_to_si_units);
					if(!b && !mstruct_new[i].equals(mstruct[i], true, true)) b = true;
				}
			}
			if(b) {
				mstruct_new.childrenUpdated();
				if(mstruct.isAddition()) mstruct_new.eval(eo2);
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
			bool child_updated = false;
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
				} else if(mstruct_old.getChild(i)->size() > 0 && !aborted()) {
					mstruct_old[i - 1] = convertToOptimalUnit(mstruct_old[i - 1], eo, convert_to_si_units);
					mstruct_old.childUpdated(i);
					if(!mstruct_old[i - 1].equals(mstruct[i - 1], true, true)) child_updated = true;
				}
			}
			if(child_updated) mstruct_old.eval(eo2);
			if((!is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || is_si_units) && old_points <= 1 && !old_minus) {
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
				child_updated = false;
				for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
					if(aborted()) return mstruct_old;
					if(mstruct_new.getChild(i)->isUnit()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->unit());
					} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isInteger()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->base()->unit(), mstruct_new.getChild(i)->exponent()->number().intValue());
					} else if(mstruct_new.getChild(i)->size() > 0 && !mstruct_new.getChild(i)->containsInterval(true, true, false, 1, true) && !aborted()) {
						MathStructure m_i_old(mstruct_new[i - 1]);
						mstruct_new[i - 1] = convertToOptimalUnit(mstruct_new[i - 1], eo, convert_to_si_units);
						mstruct_new.childUpdated(i);
						if(!mstruct_new[i - 1].equals(m_i_old, true, true)) child_updated = true;
					}
				}
				bool is_converted = false;
				if(b) {
					Unit *u = getOptimalUnit(cu, false, eo.local_currency_conversion);
					if(u != cu) {
						if(eo.approximation != APPROXIMATION_EXACT || !cu->hasApproximateRelationTo(u, true)) {
							mstruct_new = convert(mstruct_new, u, eo, true);
							is_converted = true;
						}
						if(!u->isRegistered()) delete u;
					}
				}
				delete cu;
				if((!b || !is_converted) && (!convert_to_si_units || is_si_units)) {
					return mstruct_old;
				}
				if(child_updated) mstruct_new.eval(eo2);
			}
			if((eo.approximation == APPROXIMATION_EXACT && !mstruct_old.isApproximate()) && (mstruct_new.isApproximate() || (mstruct_old.containsInterval(true, true, false, 0, true) <= 0 && mstruct_new.containsInterval(true, true, false, 0, true) > 0))) return mstruct_old;
			if(mstruct_new.equals(mstruct_old, true, true)) return mstruct_old;
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
			if(new_points == 0) return mstruct_old;
			if((new_points > old_points && (!convert_to_si_units || is_si_units || !new_is_si_units)) || (new_points == old_points && (new_minus || !old_minus) && (!new_is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || !new_is_si_units))) return mstruct_old;
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
	Unit *u = getUnit(str2);
	if(!u) u = getCompositeUnit(str2);
	Variable *v = NULL;
	if(!u) v = getVariable(str2);
	if(!u && !v) {
		for(size_t i = 0; i < signs.size(); i++) {
			if(str2 == signs[i]) {
				u = getUnit(real_signs[i]);
				if(!u) v = getVariable(real_signs[i]);
				break;
			}
		}
	}
	if(v && !v->isKnown()) v = NULL;
	if(u) {
		if(to_struct) to_struct->set(u);
		mstruct.set(convert(mstruct_to_convert, u, eo2, false, false));
		b = true;
	} else if(v) {
		if(to_struct) to_struct->set(v);
		mstruct.set(convert(mstruct_to_convert, (KnownVariable*) v, eo2));
		b = true;
	} else {
		current_stage = MESSAGE_STAGE_CONVERSION_PARSING;
		CompositeUnit cu("", "temporary_composite_convert", "", str2);
		if(cu.countUnits() == 2 && cu.get(1)->referenceName() == "g" && cu.get(2)->referenceName() == "m" && str2.substr(0, 2) == "kg") {
			int exp; Prefix *p;
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
		if(to_struct) to_struct->set(cu.generateMathStructure());
		if(cu.countUnits() > 0) {
			mstruct.set(convert(mstruct_to_convert, &cu, eo2, false, false));
			b = true;
		}
	}
	if(!b) return mstruct_to_convert;
	if(!v && eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo2));
	current_stage = MESSAGE_STAGE_UNSET;
	return mstruct;
}

