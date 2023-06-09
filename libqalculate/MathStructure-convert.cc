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
#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

void gatherInformation(const MathStructure &mstruct, vector<Unit*> &base_units, vector<AliasUnit*> &alias_units, bool check_variables = false) {
	switch(mstruct.type()) {
		case STRUCT_UNIT: {
			switch(mstruct.unit()->subtype()) {
				case SUBTYPE_BASE_UNIT: {
					for(size_t i = 0; i < base_units.size(); i++) {
						if(base_units[i] == mstruct.unit()) {
							return;
						}
					}
					base_units.push_back(mstruct.unit());
					break;
				}
				case SUBTYPE_ALIAS_UNIT: {
					for(size_t i = 0; i < alias_units.size(); i++) {
						if(alias_units[i] == mstruct.unit()) {
							return;
						}
					}
					alias_units.push_back((AliasUnit*) (mstruct.unit()));
					break;
				}
				case SUBTYPE_COMPOSITE_UNIT: {
					gatherInformation(((CompositeUnit*) (mstruct.unit()))->generateMathStructure(), base_units, alias_units, check_variables);
					break;
				}
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(check_variables && mstruct.variable()->isKnown()) gatherInformation(((KnownVariable*) mstruct.variable())->get(), base_units, alias_units, check_variables);
			break;
		}
		case STRUCT_FUNCTION: {
			if(mstruct.function()->id() == FUNCTION_ID_STRIP_UNITS) break;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(!mstruct.function()->getArgumentDefinition(i + 1) || mstruct.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					gatherInformation(mstruct[i], base_units, alias_units, check_variables);
				}
			}
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				gatherInformation(mstruct[i], base_units, alias_units, check_variables);
			}
			break;
		}
	}

}

int MathStructure::isUnitCompatible(const MathStructure &mstruct) const {
	if(!isMultiplication() && mstruct.isMultiplication()) return mstruct.isUnitCompatible(*this);
	int b1 = mstruct.containsRepresentativeOfType(STRUCT_UNIT, true, true);
	int b2 = containsRepresentativeOfType(STRUCT_UNIT, true, true);
	if(b1 < 0 || b2 < 0) return -1;
	if(b1 != b2) return false;
	if(!b1) return true;
	if(isMultiplication()) {
		size_t unit_count1 = 0, unit_count2 = 0;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isUnit_exp()) unit_count1++;
			else if(CHILD(i).containsRepresentativeOfType(STRUCT_UNIT, true, true) != 0) return -1;
		}
		if(mstruct.isMultiplication()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isUnit_exp()) unit_count2++;
				else if(mstruct[i].containsRepresentativeOfType(STRUCT_UNIT, true, true) != 0) return -1;
			}
		} else if(mstruct.isUnit_exp()) {
			if(unit_count1 > 1) return false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isUnit_exp()) return CHILD(1) == mstruct;
			}
		} else {
			return -1;
		}
		if(unit_count1 != unit_count2) return false;
		size_t i2 = 0;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isUnit_exp()) {
				for(; i2 < mstruct.size(); i2++) {
					if(mstruct[i2].isUnit_exp()) {
						if(CHILD(i) != mstruct[i2]) return false;
						i2++;
						break;
					}
				}
			}
		}
	} else if(isUnit_exp()) {
		if(mstruct.isUnit_exp()) return equals(mstruct);
	}
	return -1;
}

bool MathStructure::syncUnits(bool sync_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	if(SIZE == 0) return false;
	vector<Unit*> base_units;
	vector<AliasUnit*> alias_units;
	vector<CompositeUnit*> composite_units;
	gatherInformation(*this, base_units, alias_units);
	if(alias_units.empty() || base_units.size() + alias_units.size() == 1) return false;
	CompositeUnit *cu;
	bool b = false, do_erase = false;
	for(size_t i = 0; i < alias_units.size(); ) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			b = false;
			cu = (CompositeUnit*) alias_units[i]->baseUnit();
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(cu->containsRelativeTo(base_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
			for(size_t i2 = 0; !do_erase && i2 < alias_units.size(); i2++) {
				if(i != i2 && alias_units[i2]->baseUnit() != cu && cu->containsRelativeTo(alias_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			for(int i2 = 1; i2 <= (int) cu->countUnits(); i2++) {
				b = false;
				Unit *cub = cu->get(i2);
				switch(cub->subtype()) {
					case SUBTYPE_BASE_UNIT: {
						for(size_t i3 = 0; i3 < base_units.size(); i3++) {
							if(base_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) base_units.push_back(cub);
						break;
					}
					case SUBTYPE_ALIAS_UNIT: {
						for(size_t i3 = 0; i3 < alias_units.size(); i3++) {
							if(alias_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) alias_units.push_back((AliasUnit*) cub);
						break;
					}
					case SUBTYPE_COMPOSITE_UNIT: {
						gatherInformation(((CompositeUnit*) cub)->generateMathStructure(), base_units, alias_units);
						break;
					}
				}
			}
			i = 0;
		} else {
			i++;
		}
	}

	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		for(size_t i2 = 0; i2 < alias_units.size(); i2++) {
			if(i != i2 && alias_units[i]->baseUnit() == alias_units[i2]->baseUnit()) {
				if(alias_units[i2]->isParentOf(alias_units[i])) {
					do_erase = true;
					break;
				} else if(!alias_units[i]->isParentOf(alias_units[i2])) {
					b = false;
					for(size_t i3 = 0; i3 < base_units.size(); i3++) {
						if(base_units[i3] == alias_units[i2]->firstBaseUnit()) {
							b = true;
							break;
						}
					}
					if(!b) base_units.push_back((Unit*) alias_units[i]->baseUnit());
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(alias_units[i]->baseUnit() == base_units[i2]) {
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	b = false;
	bool fcr = false;
	for(size_t i = 0; i < composite_units.size(); i++) {
		if(convert(composite_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	if(dissolveAllCompositeUnits()) b = true;
	for(size_t i = 0; i < base_units.size(); i++) {
		if(convert(base_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	for(size_t i = 0; i < alias_units.size(); i++) {
		if(convert(alias_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	//if(b && sync_nonlinear_relations && fcr) CALCULATOR->error(false, _("Calculations involving conversion of units without proportional linear relationship might give unexpected results and is not recommended."), NULL);
	if(fcr && found_nonlinear_relations) *found_nonlinear_relations = fcr;
	return b;
}

bool has_approximate_relation_to_base(Unit *u, bool do_intervals) {
	if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
		if(((AliasUnit*) u)->isApproximate()) return do_intervals;
		if(((AliasUnit*) u)->expression().find_first_not_of(NUMBER_ELEMENTS EXPS) != string::npos && !((AliasUnit*) u)->hasNonlinearExpression()) return true;
		return has_approximate_relation_to_base(((AliasUnit*) u)->firstBaseUnit());
	} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		for(size_t i = 1; i <= ((CompositeUnit*) u)->countUnits(); i++) {
			if(has_approximate_relation_to_base(((CompositeUnit*) u)->get(i))) return true;
		}
	}
	return false;
}

bool MathStructure::testDissolveCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				if(convert(o_unit->baseUnit())) {
					convert(u);
					return true;
				}
			}
		}
	}
	return false;
}
bool test_dissolve_cu(MathStructure &mstruct, Unit *u, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, Prefix *new_prefix = NULL) {
	if(mstruct.isUnit()) {
		if(mstruct.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) mstruct.unit())->containsRelativeTo(u)) {
				mstruct.set(((CompositeUnit*) mstruct.unit())->generateMathStructure());
				return true;
			}
		} else if(mstruct.unit()->subtype() == SUBTYPE_ALIAS_UNIT && mstruct.unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (mstruct.unit()->baseUnit()))->containsRelativeTo(u)) {
				if(mstruct.convert(mstruct.unit()->baseUnit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) {
					mstruct.convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
					return true;
				}
			}
		}
	}
	return false;
}
bool MathStructure::testCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				return true;
			}
		}
	}
	return false;
}
bool MathStructure::dissolveAllCompositeUnits() {
	switch(m_type) {
		case STRUCT_UNIT: {
			if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
			break;
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).dissolveAllCompositeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
	}
	return false;
}
bool MathStructure::setPrefixForUnit(Unit *u, Prefix *new_prefix) {
	if(m_type == STRUCT_UNIT && o_unit == u) {
		if(o_prefix != new_prefix) {
			Number new_value(1, 1);
			if(o_prefix) new_value = o_prefix->value();
			if(new_prefix) new_value.divide(new_prefix->value());
			o_prefix = new_prefix;
			multiply(new_value);
			return true;
		}
		return false;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).setPrefixForUnit(u, new_prefix)) {
			CHILD_UPDATED(i);
			b = true;
		}
	}
	return b;
}

bool searchSubMultiplicationsForComplexRelations(Unit *u, const MathStructure &mstruct) {
	int b_c = -1;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isUnit_exp()) {
			if((mstruct[i].isUnit() && u->hasNonlinearRelationTo(mstruct[i].unit())) || (mstruct[i].isPower() && u->hasNonlinearRelationTo(mstruct[i][0].unit()))) {
				return true;
			}
		} else if(b_c == -1 && mstruct[i].isMultiplication()) {
			b_c = -3;
		}
	}
	if(b_c == -3) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication() && searchSubMultiplicationsForComplexRelations(u, mstruct[i])) return true;
		}
	}
	return false;
}
bool MathStructure::convertToBaseUnits(bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, bool avoid_approximate_variables) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			set(((CompositeUnit*) o_unit)->generateMathStructure());
			convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
			return true;
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT) {
			AliasUnit *au = (AliasUnit*) o_unit;
			if(au->hasNonlinearRelationTo(au->baseUnit())) {
				if(found_nonlinear_relations) *found_nonlinear_relations = true;
				if(!convert_nonlinear_relations) {
					if(!au->hasNonlinearExpression() && ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) || !au->hasApproximateExpression(avoid_approximate_variables, false))) {
						MathStructure mstruct_old(*this);
						if(convert(au->firstBaseUnit(), false, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
							convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables);
							return true;
						}
					}
					return false;
				}
			}
			if((feo.approximation == APPROXIMATION_EXACT || feo.approximation == APPROXIMATION_EXACT_VARIABLES) && au->hasApproximateRelationTo(au->baseUnit(), avoid_approximate_variables, false)) {
				MathStructure mstruct_old(*this);
				if(convert(au->firstBaseUnit(), false, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
					convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables);
					return true;
				}
				return false;
			}
			if(convert(au->baseUnit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) {
				convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
				return true;
			}
		}
		return false;
	} else if(m_type == STRUCT_MULTIPLICATION && (convert_nonlinear_relations || found_nonlinear_relations)) {
		AliasUnit *complex_au = NULL;
		if(convert_nonlinear_relations && convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables)) {
			convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
			return true;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return false;
			if(CHILD(i).isUnit_exp() && CHILD(i).unit_exp_unit()->subtype() == SUBTYPE_ALIAS_UNIT) {
				AliasUnit *au = (AliasUnit*) CHILD(i).unit_exp_unit();
				if(au && au->hasNonlinearRelationTo(au->baseUnit())) {
					if(found_nonlinear_relations) {
						*found_nonlinear_relations = true;
					}
					if(convert_nonlinear_relations) {
						if(complex_au) {
							complex_au = NULL;
							convert_nonlinear_relations = false;
							break;
						} else {
							complex_au = au;
						}
					} else {
						break;
					}
				}
			}
		}
		if(convert_nonlinear_relations && complex_au && ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) || !complex_au->hasApproximateExpression(avoid_approximate_variables, false))) {
			MathStructure mstruct_old(*this);
			if(convert(complex_au->firstBaseUnit(), true, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
				convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
				return true;
			}
		}
	} else if(m_type == STRUCT_FUNCTION) {
		if(o_function->id() == FUNCTION_ID_STRIP_UNITS) return false;
		bool b = false;
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return b;
			if((!o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && CHILD(i).convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CALCULATOR->aborted()) return b;
		if(CHILD(i).convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables)) {
			CHILD_UPDATED(i);
			b = true;
		}
	}
	return b;
}
bool convert_approximate(MathStructure &m, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals);
bool convert_approximate(MathStructure &m, const MathStructure unit_mstruct, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	bool b = false;
	if(unit_mstruct.type() == STRUCT_UNIT) {
		if(convert_approximate(m, unit_mstruct.unit(), feo, vars, uncs, units, do_intervals)) b = true;
	} else {
		for(size_t i = 0; i < unit_mstruct.size(); i++) {
			if(convert_approximate(m, unit_mstruct[i], feo, vars, uncs, units, do_intervals)) b = true;
		}
	}
	return b;
}
bool test_dissolve_cu_approximate(MathStructure &mstruct, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	if(mstruct.isUnit()) {
		if(mstruct.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) mstruct.unit())->containsRelativeTo(u)) {
				mstruct.set(((CompositeUnit*) mstruct.unit())->generateMathStructure());
				return true;
			}
		} else if(mstruct.unit()->subtype() == SUBTYPE_ALIAS_UNIT && mstruct.unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (mstruct.unit()->baseUnit()))->containsRelativeTo(u)) {
				if(convert_approximate(mstruct, mstruct.unit()->baseUnit(), feo, vars, uncs, units, do_intervals)) {
					convert_approximate(mstruct, u, feo, vars, uncs, units, do_intervals);
					return true;
				}
			}
		}
	}
	return false;
}
bool convert_approximate(MathStructure &m, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	bool b = false;
	if(m.type() == STRUCT_UNIT && (m.unit() == u || m.prefix())) {
		return false;
	}
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && !(m.type() == STRUCT_UNIT && m.unit()->baseUnit() == u)) {
		return convert_approximate(m, ((CompositeUnit*) u)->generateMathStructure(false, true), feo, vars, uncs, units, do_intervals);
	}
	if(m.type() == STRUCT_UNIT) {
		if(u->hasApproximateRelationTo(m.unit(), feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES, true) || u->hasNonlinearRelationTo(m.unit())) {
			return false;
		}
		if(!vars) return true;
		if(m.unit()->baseUnit() != u->baseUnit() && test_dissolve_cu_approximate(m, u, feo, vars, uncs, units, do_intervals)) {
			convert_approximate(m, u, feo, vars, uncs, units, do_intervals);
			return true;
		}
		MathStructure *exp = new MathStructure(1, 1, 0);
		MathStructure *mstruct = new MathStructure(1, 1, 0);
		Unit *u_m = m.unit();
		KnownVariable *v = NULL;
		if(u_m->subtype() == SUBTYPE_ALIAS_UNIT && do_intervals) {
			for(size_t i = 0; i < units->size(); i++) {
				if((*units)[i] == u_m) {
					mstruct->set((*vars)[i]);
					u_m = ((AliasUnit*) u_m)->firstBaseUnit();
				}
			}
			if(u_m == m.unit()) {
				((AliasUnit*) u_m)->convertToFirstBaseUnit(*mstruct, *exp);
				if(mstruct->isNumber() && mstruct->number().isInterval()) {
					uncs->push_back(mstruct->number().uncertainty());
					units->push_back(u_m);
					Number nmid(mstruct->number());
					nmid.intervalToMidValue();
					nmid.setApproximate(false);
					v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
					mstruct->set(v);
					v->ref();
					v->destroy();
					u_m = ((AliasUnit*) u_m)->firstBaseUnit();
				} else {
					mstruct->set(1, 1, 0);
					exp->set(1, 1, 0);
				}
			}
		}
		if(u->convert(u_m, *mstruct, *exp) && (do_intervals || (!mstruct->containsInterval(true, false, false, 1, true) && !exp->containsInterval(true, false, false, 1, true)))) {
			m.setUnit(u);
			if(!exp->isOne()) {
				if(feo.calculate_functions) {
					calculate_nondifferentiable_functions(*exp, feo, true, true, feo.interval_calculation != INTERVAL_CALCULATION_VARIANCE_FORMULA ? 0 : ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) ? 2 : 1));
				}
				if(do_intervals) exp->calculatesub(feo, feo, true);
				m.raise_nocopy(exp);
			} else {
				exp->unref();
			}
			if(!mstruct->isOne()) {
				if(feo.calculate_functions) {
					calculate_nondifferentiable_functions(*mstruct, feo, true, true, feo.interval_calculation != INTERVAL_CALCULATION_VARIANCE_FORMULA ? 0 : ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) ? 2 : 1));
				}
				if(do_intervals) mstruct->calculatesub(feo, feo, true);
				m.multiply_nocopy(mstruct);
			} else {
				mstruct->unref();
			}
			if(v) {
				if(v->refcount() == 1) {
					v->unref();
					units->pop_back();
					uncs->pop_back();
				} else {
					vars->push_back(v);
				}
			}
			return true;
		} else {
			exp->unref();
			mstruct->unref();
			if(v) {
				v->unref();
				units->pop_back();
				uncs->pop_back();
			}
			return false;
		}
	} else {
		if(m.type() == STRUCT_FUNCTION) {
			if(m.function()->id() == FUNCTION_ID_STRIP_UNITS) return b;
			for(size_t i = 0; i < m.size(); i++) {
				if(m.size() > 100 && CALCULATOR->aborted()) return b;
				if((!m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && convert_approximate(m[i], u, feo, vars, uncs, units, do_intervals)) {
					m.childUpdated(i + 1);
					b = true;
				}
			}
			return b;
		}
		for(size_t i = 0; i < m.size(); i++) {
			if(m.size() > 100 && CALCULATOR->aborted()) return b;
			if(convert_approximate(m[i], u, feo, vars, uncs, units, do_intervals)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
		return b;
	}
	return b;
}

bool contains_approximate_relation_to_base(const MathStructure &m, bool do_intervals) {
	if(m.isUnit()) {
		return has_approximate_relation_to_base(m.unit(), do_intervals);
	} else if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) {
		return false;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_approximate_relation_to_base(m[i], do_intervals)) return true;
	}
	return false;
}

bool sync_approximate_units(MathStructure &m, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, bool do_intervals) {
	if(m.size() == 0) return false;
	if(!contains_approximate_relation_to_base(m, do_intervals)) return false;
	bool check_variables = (feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES);
	vector<Unit*> base_units;
	vector<AliasUnit*> alias_units;
	vector<CompositeUnit*> composite_units;
	gatherInformation(m, base_units, alias_units, check_variables);
	if(alias_units.empty() || base_units.size() + alias_units.size() == 1) return false;
	CompositeUnit *cu;
	bool b = false, do_erase = false;
	for(size_t i = 0; i < alias_units.size(); ) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			b = false;
			cu = (CompositeUnit*) alias_units[i]->baseUnit();
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(cu->containsRelativeTo(base_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
			for(size_t i2 = 0; !do_erase && i2 < alias_units.size(); i2++) {
				if(i != i2 && alias_units[i2]->baseUnit() != cu && cu->containsRelativeTo(alias_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			for(int i2 = 1; i2 <= (int) cu->countUnits(); i2++) {
				b = false;
				Unit *cub = cu->get(i2);
				switch(cub->subtype()) {
					case SUBTYPE_BASE_UNIT: {
						for(size_t i3 = 0; i3 < base_units.size(); i3++) {
							if(base_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) base_units.push_back(cub);
						break;
					}
					case SUBTYPE_ALIAS_UNIT: {
						for(size_t i3 = 0; i3 < alias_units.size(); i3++) {
							if(alias_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) alias_units.push_back((AliasUnit*) cub);
						break;
					}
					case SUBTYPE_COMPOSITE_UNIT: {
						gatherInformation(((CompositeUnit*) cub)->generateMathStructure(), base_units, alias_units, check_variables);
						break;
					}
				}
			}
			i = 0;
		} else {
			i++;
		}
	}

	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		for(size_t i2 = 0; i2 < alias_units.size(); i2++) {
			if(i != i2 && alias_units[i]->baseUnit() == alias_units[i2]->baseUnit()) {
				if(alias_units[i2]->isParentOf(alias_units[i])) {
					do_erase = true;
					break;
				} else if(!alias_units[i]->isParentOf(alias_units[i2])) {
					b = false;
					for(size_t i3 = 0; i3 < base_units.size(); i3++) {
						if(base_units[i3] == alias_units[i2]->firstBaseUnit()) {
							b = true;
							break;
						}
					}
					if(!b) base_units.push_back((Unit*) alias_units[i]->baseUnit());
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(alias_units[i]->baseUnit() == base_units[i2]) {
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	b = false;
	vector<Unit*> units;
	for(size_t i = 0; i < composite_units.size(); i++) {
		if(convert_approximate(m, composite_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	if(m.dissolveAllCompositeUnits()) b = true;
	for(size_t i = 0; i < base_units.size(); i++) {
		if(convert_approximate(m, base_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	for(size_t i = 0; i < alias_units.size(); i++) {
		if(convert_approximate(m, alias_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	return b;
}


bool MathStructure::convert(Unit *u, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, Prefix *new_prefix) {
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return false;
	bool b = false;
	if(m_type == STRUCT_UNIT && o_unit == u) {
		if((new_prefix || o_prefix) && o_prefix != new_prefix) {
			Number new_value(1, 1);
			if(o_prefix) new_value = o_prefix->value();
			if(new_prefix) new_value.divide(new_prefix->value());
			o_prefix = new_prefix;
			multiply(new_value);
			return true;
		}
		return false;
	}
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && !(m_type == STRUCT_UNIT && o_unit->baseUnit() == u)) {
		return convert(((CompositeUnit*) u)->generateMathStructure(false, true), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo);
	}
	if(m_type == STRUCT_UNIT) {
		if(u->hasNonlinearRelationTo(o_unit)) {
			if(found_nonlinear_relations) *found_nonlinear_relations = true;
			if(!convert_nonlinear_relations) return false;
		}
		if(o_unit->baseUnit() != u->baseUnit() && test_dissolve_cu(*this, u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
			convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
			return true;
		}
		MathStructure *exp = new MathStructure(1, 1, 0);
		MathStructure *mstruct = new MathStructure(1, 1, 0);
		if(o_prefix) {
			mstruct->set(o_prefix->value());
		}
		if(u->convert(o_unit, *mstruct, *exp) && (feo.approximation != APPROXIMATION_EXACT || (!mstruct->isApproximate() && !exp->isApproximate()))) {
			setUnit(u);
			o_prefix = new_prefix;
			if(new_prefix) {
				divide(new_prefix->value());
			}
			if(!exp->isOne()) {
				if(calculate_new_functions) exp->calculateFunctions(feo, true, false);
				raise_nocopy(exp);
			} else {
				exp->unref();
			}
			if(!mstruct->isOne()) {
				if(calculate_new_functions) mstruct->calculateFunctions(feo, true, false);
				multiply_nocopy(mstruct);
			} else {
				mstruct->unref();
			}
			return true;
		} else {
			exp->unref();
			mstruct->unref();
			return false;
		}
	} else {
		if(convert_nonlinear_relations || found_nonlinear_relations) {
			if(m_type == STRUCT_MULTIPLICATION) {
				long int b_c = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isUnit_exp()) {
						Unit *u2 = CHILD(i).isPower() ? CHILD(i)[0].unit() : CHILD(i).unit();
						if(u->hasNonlinearRelationTo(u2)) {
							if(found_nonlinear_relations) *found_nonlinear_relations = true;

							b_c = i;
							break;
						}
					}
				}
				if(convert_nonlinear_relations && b_c >= 0) {
					MathStructure mstruct(1, 1);
					MathStructure mstruct2(1, 1);
					if(SIZE == 2) {
						if(b_c == 0) mstruct = CHILD(1);
						else mstruct = CHILD(0);
						if(mstruct.containsType(STRUCT_UNIT, false, true, true) > 0) {
							mstruct2 = mstruct;
							mstruct.set(1, 1, 0);
						}
					} else if(SIZE > 2) {
						mstruct = *this;
						size_t nr_of_del = 0;
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).containsType(STRUCT_UNIT, false, true, true) > 0) {
								mstruct.delChild(i + 1 - nr_of_del);
								nr_of_del++;
								if((long int) i != b_c) {
									if(mstruct2.isOne()) mstruct2 = CHILD(i);
									else mstruct2.multiply(CHILD(i), true);
								}
							}
						}
						if(mstruct.size() == 1) mstruct.setToChild(1);
						else if(mstruct.size() == 0) mstruct.set(1, 1, 0);
					}
					MathStructure exp(1, 1);
					Unit *u2;
					bool b_p = false;
					if(CHILD(b_c).isPower()) {
						if(CHILD(b_c)[0].unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(b_c)[0].unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(b_c)[0].unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(b_c)[0].unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								if(!convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) return false;
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						exp = CHILD(b_c)[1];
						u2 = CHILD(b_c)[0].unit();
						if(CHILD(b_c)[0].prefix() && CHILD(b_c)[0].prefix() != CALCULATOR->getDecimalNullPrefix() && CHILD(b_c)[0].prefix() != CALCULATOR->getBinaryNullPrefix()) b_p = true;
					} else {
						if(CHILD(b_c).unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(b_c).unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(b_c).unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(b_c).unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								if(!convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) return false;
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						u2 = CHILD(b_c).unit();
						if(CHILD(b_c).prefix() && CHILD(b_c).prefix() != CALCULATOR->getDecimalNullPrefix() && CHILD(b_c).prefix() != CALCULATOR->getBinaryNullPrefix()) b_p = true;
					}
					size_t efc = 0, mfc = 0;
					if(calculate_new_functions) {
						efc = exp.countFunctions();
						mfc = mstruct.countFunctions();
					}
					Unit *u1 = u;
					if((!mstruct2.isOne() || !exp.isOne()) && (CALCULATOR->getTemperatureCalculationMode() == TEMPERATURE_CALCULATION_RELATIVE && u->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_KELVIN))) {
						if(u2 == CALCULATOR->getUnitById(UNIT_ID_CELSIUS)) u2 = CALCULATOR->getUnitById(UNIT_ID_KELVIN);
						else if(u2 == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT) && CALCULATOR->getUnitById(UNIT_ID_RANKINE)) u2 = CALCULATOR->getUnitById(UNIT_ID_RANKINE);
						if(u1 == CALCULATOR->getUnitById(UNIT_ID_CELSIUS)) u1 = CALCULATOR->getUnitById(UNIT_ID_KELVIN);
						else if(u1 == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT) && CALCULATOR->getUnitById(UNIT_ID_RANKINE)) u1 = CALCULATOR->getUnitById(UNIT_ID_RANKINE);
					}
					if(u1->convert(u2, mstruct, exp)) {
						if(feo.approximation == APPROXIMATION_EXACT && !isApproximate() && (mstruct.isApproximate() || exp.isApproximate())) return false;
						if(b_p) {
							EvaluationOptions eo = feo;
							eo.keep_prefixes = false;
							unformat(eo);
							return convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
						}
						set(u);
						if(!exp.isOne()) {
							if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true, false);
							raise(exp);
						}
						if(!mstruct2.isOne()) {
							multiply(mstruct2);
						}
						if(!mstruct.isOne()) {
							if(calculate_new_functions && mstruct.countFunctions() > mfc) mstruct.calculateFunctions(feo, true, false);
							multiply(mstruct);
						}
						if(u->baseUnit() != CALCULATOR->getUnitById(UNIT_ID_KELVIN)) convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
						return true;
					}
					return false;
				}
			} else if(m_type == STRUCT_POWER) {
				if(CHILD(0).isUnit() && u->hasNonlinearRelationTo(CHILD(0).unit())) {
					if(found_nonlinear_relations) *found_nonlinear_relations = true;
					if(convert_nonlinear_relations) {
						if(CHILD(0).unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(0).unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(0).unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(0).unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								if(!convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) return false;
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						MathStructure exp(CHILD(1));
						MathStructure mstruct(1, 1);
						if(CHILD(0).prefix()) {
							mstruct.set(CHILD(0).prefix()->value());
							mstruct.raise(exp);
						}
						size_t efc = 0;
						if(calculate_new_functions) {
							efc = exp.countFunctions();
						}
						if(u->convert(CHILD(0).unit(), mstruct, exp)) {
							if(feo.approximation == APPROXIMATION_EXACT && !isApproximate() && (mstruct.isApproximate() || exp.isApproximate())) return false;
							set(u);
							if(!exp.isOne()) {
								if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true, false);
								raise(exp);
							}
							if(!mstruct.isOne()) {
								if(calculate_new_functions) mstruct.calculateFunctions(feo, true, false);
								multiply(mstruct);
							}
							return true;
						}
						return false;
					}
				}
			}
		}
		if(m_type == STRUCT_FUNCTION) {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS) return b;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return b;
				if((!o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && CHILD(i).convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return b;
			if(CHILD(i).convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	return b;
}
bool MathStructure::convert(const MathStructure unit_mstruct, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	bool b = false;
	if(unit_mstruct.type() == STRUCT_UNIT) {
		if(convert(unit_mstruct.unit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, feo.keep_prefixes ? unit_mstruct.prefix() : NULL)) b = true;
	} else {
		for(size_t i = 0; i < unit_mstruct.size(); i++) {
			if(convert(unit_mstruct[i], convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) b = true;
		}
	}
	return b;
}

