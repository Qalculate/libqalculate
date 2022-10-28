/*
    Qalculate (libary)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Unit.h"
#include "util.h"
#include "Calculator.h"
#include "MathStructure.h"
#include "MathStructure-support.h"
#include "Prefix.h"
#include "Variable.h"
#include "BuiltinFunctions.h"

using std::string;
using std::vector;
using std::cout;
using std::endl;

Unit::Unit(string cat_, string name_, string plural_, string singular_, string title_, bool is_local, bool is_builtin, bool is_active) : ExpressionItem(cat_, "", title_, "", is_local, is_builtin, is_active) {
	remove_blank_ends(plural_);
	remove_blank_ends(singular_);
	if(!name_.empty()) {
		names.resize(1);
		names[0].name = name_;
		names[0].unicode = false;
		names[0].abbreviation = true;
		names[0].case_sensitive = true;
		size_t i = name_.find('_');
		if(i != string::npos && i > 0 && i < name_.length() - 1 && name_.find('_', i + 1) == string::npos) names[0].suffix = true;
		else names[0].suffix = false;
		names[0].avoid_input = false;
		names[0].reference = true;
		names[0].plural = false;
	}
	if(!singular_.empty()) {
		names.resize(names.size() + 1);
		names[names.size() - 1].name = singular_;
		names[names.size() - 1].unicode = false;
		names[names.size() - 1].abbreviation = false;
		names[names.size() - 1].case_sensitive = text_length_is_one(names[names.size() - 1].name);
		names[names.size() - 1].suffix = false;
		names[names.size() - 1].avoid_input = false;
		names[names.size() - 1].reference = false;
		names[names.size() - 1].plural = false;
	}
	if(!plural_.empty()) {
		names.resize(names.size() + 1);
		names[names.size() - 1].name = plural_;
		names[names.size() - 1].unicode = false;
		names[names.size() - 1].abbreviation = false;
		names[names.size() - 1].case_sensitive = text_length_is_one(names[names.size() - 1].name);
		names[names.size() - 1].suffix = false;
		names[names.size() - 1].avoid_input = false;
		names[names.size() - 1].reference = false;
		names[names.size() - 1].plural = true;
	}
	b_si = false;
	b_use_with_prefixes = false;
}
Unit::Unit() {
	b_si = false;
	b_use_with_prefixes = false;
}
Unit::Unit(const Unit *unit) {
	set(unit);
}
Unit::~Unit() {}
ExpressionItem *Unit::copy() const {
	return new Unit(this);
}
void Unit::set(const ExpressionItem *item) {
	if(item->type() == TYPE_UNIT) {
		b_si = ((Unit*) item)->isSIUnit();
		ssystem = ((Unit*) item)->system();
		scountries = ((Unit*) item)->countries();
	}
	ExpressionItem::set(item);
}
bool Unit::isSIUnit() const {
	return b_si;
}
void Unit::setAsSIUnit() {
	if(!b_si) {
		b_si = true;
		b_use_with_prefixes = true;
		ssystem = "SI";
		setChanged(true);
	}
}
void Unit::setSystem(string s_system) {
	if(s_system != ssystem) {
		ssystem = s_system;
		if(ssystem == "SI" || ssystem == "si" || ssystem == "Si") {
			b_si = true;
			b_use_with_prefixes = true;
		} else {
			b_si = false;
		}
		setChanged(true);
	}
}
const string &Unit::system() const {
	return ssystem;
}
bool Unit::useWithPrefixesByDefault() const {
	return b_use_with_prefixes;
}
void Unit::setUseWithPrefixesByDefault(bool use_with_prefixes) {
	b_use_with_prefixes = use_with_prefixes;
}
bool Unit::isCurrency() const {
	return baseUnit() == CALCULATOR->getUnitById(UNIT_ID_EURO);
}
const string &Unit::countries() const {return scountries;}
void Unit::setCountries(string country_names) {
	remove_blank_ends(country_names);
	if(scountries != country_names) {
		scountries = country_names;
		setChanged(true);
	}
}
bool Unit::isUsedByOtherUnits() const {
	return CALCULATOR->unitIsUsedByOtherUnits(this);
}
string Unit::print(const PrintOptions &po, bool format, int tagtype, bool input, bool plural) const {
	if(input) {
		preferredInputName(po.abbreviate_names, po.use_unicode_signs, plural, po.use_reference_names || (po.preserve_format && isCurrency()), po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(TYPE_UNIT, !po.use_reference_names && tagtype != TAG_TYPE_TERMINAL, format && tagtype == TAG_TYPE_HTML, !po.use_reference_names && !po.preserve_format, po.hide_underscore_spaces);
	}
	return preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, plural, po.use_reference_names || (po.preserve_format && isCurrency()), po.can_display_unicode_string_function, po.can_display_unicode_string_arg).formattedName(TYPE_UNIT, !po.use_reference_names && tagtype != TAG_TYPE_TERMINAL, format && tagtype == TAG_TYPE_HTML, !po.use_reference_names && !po.preserve_format, po.hide_underscore_spaces);
}
string Unit::print(bool plural_, bool short_, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	return preferredName(short_, use_unicode, plural_, false, can_display_unicode_string_function, can_display_unicode_string_arg).name;
}
const string &Unit::plural(bool return_singular_if_no_plural, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	const ExpressionName *ename = &preferredName(false, use_unicode, true, false, can_display_unicode_string_function, can_display_unicode_string_arg);
	if(!return_singular_if_no_plural && !ename->plural) return empty_string;
	return ename->name;
}
const string &Unit::singular(bool return_abbreviation_if_no_singular, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	const ExpressionName *ename = &preferredName(false, use_unicode, false, false, can_display_unicode_string_function, can_display_unicode_string_arg);
	if(!return_abbreviation_if_no_singular && ename->abbreviation) return empty_string;
	return ename->name;
}
const string &Unit::abbreviation(bool return_singular_if_no_abbreviation, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	const ExpressionName *ename = &preferredName(true, use_unicode, false, false, can_display_unicode_string_function, can_display_unicode_string_arg);
	if(!return_singular_if_no_abbreviation && !ename->abbreviation) return empty_string;
	return ename->name;
}
Unit* Unit::baseUnit() const {
	return (Unit*) this;
}
MathStructure &Unit::convertToBaseUnit(MathStructure &mvalue, MathStructure&) const {
	return mvalue;
}
MathStructure &Unit::convertFromBaseUnit(MathStructure &mvalue, MathStructure&) const {
	return mvalue;
}
MathStructure &Unit::convertToBaseUnit(MathStructure &mvalue) const {
	return mvalue;
}
MathStructure &Unit::convertFromBaseUnit(MathStructure &mvalue) const {
	return mvalue;
}
MathStructure Unit::convertToBaseUnit() const {
	return MathStructure(1, 1);
}
MathStructure Unit::convertFromBaseUnit() const {
	return MathStructure(1, 1);
}
int Unit::baseExponent(int exp) const {
	return exp;
}
int Unit::type() const {
	return TYPE_UNIT;
}
int Unit::subtype() const {
	return SUBTYPE_BASE_UNIT;
}
bool Unit::isChildOf(Unit*) const {
	return false;
}
bool Unit::isParentOf(Unit *u) const {
	return u != this && u->baseUnit() == this;
}
bool Unit::hasNonlinearRelationTo(Unit *u) const {
	if(u == this) return false;
	Unit *ub2 = u->baseUnit();
	if(ub2 != this) {
		if(subtype() == SUBTYPE_COMPOSITE_UNIT) {
			const CompositeUnit *cu = (CompositeUnit*) this;
			for(size_t i = 1; i <= cu->countUnits(); i++) {
				if(cu->get(i)->hasNonlinearRelationTo(u)) {
					return true;
				}
			}
			return false;
		}
		if(ub2->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(u->hasNonlinearRelationTo(ub2) && (((CompositeUnit*) ub2))->containsRelativeTo(baseUnit())) {
				return true;
			}
		}
		return false;
	}
	Unit *fbu = u;
	while(true) {
		if(fbu == this) return false;
		if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
		if(((AliasUnit*) fbu)->hasNonlinearExpression()) return true;
		fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
	}
}
bool Unit::hasApproximateRelationTo(Unit *u, bool check_variables, bool ignore_high_precision_intervals) const {
	if(u == this) return false;
	Unit *ub2 = u->baseUnit();
	if(ub2 != this) {
		if(subtype() == SUBTYPE_COMPOSITE_UNIT) {
			const CompositeUnit *cu = (CompositeUnit*) this;
			for(size_t i = 1; i <= cu->countUnits(); i++) {
				if(cu->get(i)->hasApproximateRelationTo(u, check_variables, ignore_high_precision_intervals)) return true;
			}
			return false;
		}
		if(ub2->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if((((CompositeUnit*) ub2))->containsRelativeTo(baseUnit()) && u->hasApproximateRelationTo(ub2, check_variables, ignore_high_precision_intervals)) return true;
		}
		return false;
	}
	Unit *fbu = u;
	while(true) {
		if(fbu == this) return false;
		if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
		if(((AliasUnit*) fbu)->hasApproximateExpression(check_variables, ignore_high_precision_intervals)) return true;
		fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
	}
}
bool Unit::containsRelativeTo(Unit *u) const {return false;}
bool Unit::hasNonlinearRelationToBase() const {return false;}
bool Unit::hasApproximateRelationToBase(bool, bool) const {return false;}

MathStructure Unit::convert(Unit *u, bool *converted) const {
	MathStructure mexp(1, 1);
	MathStructure mvalue(1, 1);
	bool b = convert(u, mvalue, mexp);
	if(converted) *converted = b;
	return mvalue;
}
bool Unit::convert(Unit *u, MathStructure &mvalue) const {
	MathStructure mexp(1, 1);
	return convert(u, mvalue, mexp);
}
bool Unit::convert(Unit *u, MathStructure &mvalue, MathStructure &mexp) const {
	if(u == this) {
		return true;
	} else if(u->baseUnit() == baseUnit()) {
		u->convertToBaseUnit(mvalue, mexp);
		convertFromBaseUnit(mvalue, mexp);
		if(isCurrency() && u->isCurrency()) {
			int i = 0;
			if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->isBuiltin()) {
				Unit *u_parent = ((AliasUnit*) u)->firstBaseUnit();
				if(u == CALCULATOR->getUnitById(UNIT_ID_BTC) || u_parent == CALCULATOR->getUnitById(UNIT_ID_BTC)) {
					if(u == CALCULATOR->getUnitById(UNIT_ID_BTC) || this != CALCULATOR->getUnitById(UNIT_ID_BTC)) i = i | 0b0010;
				} else if(u == CALCULATOR->getUnitById(UNIT_ID_BYN) || u_parent == CALCULATOR->getUnitById(UNIT_ID_BYN)) {
					if(u == CALCULATOR->getUnitById(UNIT_ID_BYN) || this != CALCULATOR->getUnitById(UNIT_ID_BYN)) i = i | 0b1000;
				} else if(u_parent == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					if(subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) this)->firstBaseUnit() != u) i = i | 0b0001;
				} else {
					if(this == CALCULATOR->getUnitById(UNIT_ID_EURO)) i = i | 0b0001;
					i = i | 0b0100;
				}
			}
			if(subtype() == SUBTYPE_ALIAS_UNIT && isBuiltin()) {
				Unit *u_parent = ((AliasUnit*) this)->firstBaseUnit();
				if(this == CALCULATOR->getUnitById(UNIT_ID_BTC) || u_parent == CALCULATOR->getUnitById(UNIT_ID_BTC)) {
					if(this == CALCULATOR->getUnitById(UNIT_ID_BTC) || u != CALCULATOR->getUnitById(UNIT_ID_BTC)) {
						if(i & 0b0100) i = i | 0b0001;
						i = i | 0b0010;
					}
				} else if(this == CALCULATOR->getUnitById(UNIT_ID_BYN) || u_parent == CALCULATOR->getUnitById(UNIT_ID_BYN)) {
					if(this == CALCULATOR->getUnitById(UNIT_ID_BYN) || u != CALCULATOR->getUnitById(UNIT_ID_BYN)) {
						if(i & 0b0100) i = i | 0b0001;
						i = i | 0b1000;
					}
				} else if(u_parent == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					if(u->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) u)->firstBaseUnit() != this) i = i | 0b0001;
				} else {
					if((i & 0b1000) || (i & 0b0010) || u == CALCULATOR->getUnitById(UNIT_ID_EURO)) i = i | 0b0001;
					i = i | 0b0100;
				}
			}
			CALCULATOR->setExchangeRatesUsed(i);
		}
		return true;
	}
	return false;
}

AliasUnit::AliasUnit(string cat_, string name_, string plural_, string short_name_, string title_, Unit *alias, string relation, int exp, string inverse, bool is_local, bool is_builtin, bool is_active) : Unit(cat_, name_, plural_, short_name_, title_, is_local, is_builtin, is_active) {
	o_unit = (Unit*) alias;
	remove_blank_ends(relation);
	remove_blank_ends(inverse);
	svalue = relation;
	sinverse = inverse;
	suncertainty = "";
	b_relative_uncertainty = false;
	i_exp = exp;
	i_mix = 0;
	i_mix_min = 0;
}
AliasUnit::AliasUnit() {
	o_unit = NULL;
	svalue = "";
	sinverse = "";
	suncertainty = "";
	b_relative_uncertainty = false;
	i_exp = 1;
	i_mix = 0;
	i_mix_min = 0;
}
AliasUnit::AliasUnit(const AliasUnit *unit) {
	set(unit);
}
AliasUnit::~AliasUnit() {}
ExpressionItem *AliasUnit::copy() const {
	return new AliasUnit(this);
}
void AliasUnit::set(const ExpressionItem *item) {
	if(item->type() == TYPE_UNIT) {
		Unit::set(item);
		if(((Unit*) item)->subtype() == SUBTYPE_ALIAS_UNIT) {
			AliasUnit *u = (AliasUnit*) item;
			o_unit = (Unit*) u->firstBaseUnit();
			i_exp = u->firstBaseExponent();
			svalue = u->expression();
			sinverse = u->inverseExpression();
			suncertainty = u->uncertainty(&b_relative_uncertainty);
			i_mix = u->mixWithBase();
			i_mix_min = u->mixWithBaseMinimum();
		}
	} else {
		ExpressionItem::set(item);
	}
}
Unit* AliasUnit::baseUnit() const {
	return o_unit->baseUnit();
}
Unit* AliasUnit::firstBaseUnit() const {
	return o_unit;
}
void AliasUnit::setBaseUnit(Unit *alias) {
	o_unit = (Unit*) alias;
	setChanged(true);
}
string AliasUnit::expression() const {
	return svalue;
}
string AliasUnit::inverseExpression() const {
	return sinverse;
}
string AliasUnit::uncertainty(bool *is_relative) const {
	if(is_relative) *is_relative = b_relative_uncertainty;
	return suncertainty;
}
void AliasUnit::setExpression(string relation) {
	remove_blank_ends(relation);
	if(relation.empty()) {
		svalue = "1";
	} else {
		svalue = relation;
	}
	setChanged(true);
}
void AliasUnit::setInverseExpression(string inverse) {
	remove_blank_ends(inverse);
	sinverse = inverse;
	setChanged(true);
}
void AliasUnit::setUncertainty(string standard_uncertainty, bool is_relative) {
	remove_blank_ends(standard_uncertainty);
	suncertainty = standard_uncertainty;
	b_relative_uncertainty = is_relative;
	if(!suncertainty.empty()) setApproximate(true);
	setChanged(true);
}
MathStructure &AliasUnit::convertToBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	convertToFirstBaseUnit(mvalue, mexp);
	return o_unit->convertToBaseUnit(mvalue, mexp);
}
MathStructure &AliasUnit::convertFromBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	Unit *u = (Unit*) baseUnit();
	AliasUnit *u2;
	while(true) {
		u2 = (AliasUnit*) this;
		while(true) {
			if(u2->firstBaseUnit() == u) {
				break;
			} else {
				u2 = (AliasUnit*) u2->firstBaseUnit();
			}
		}
		u = u2;
		u2->convertFromFirstBaseUnit(mvalue, mexp);
		if(u == this) break;
	}
	return mvalue;
}
MathStructure &AliasUnit::convertToBaseUnit(MathStructure &mvalue) const {
	MathStructure mexp(1, 1);
	return convertToBaseUnit(mvalue, mexp);
}
MathStructure &AliasUnit::convertFromBaseUnit(MathStructure &mvalue) const {
	MathStructure mexp(1, 1);
	return convertFromBaseUnit(mvalue, mexp);
}
MathStructure AliasUnit::convertToBaseUnit() const {
	MathStructure mexp(1, 1);
	MathStructure mvalue(1, 1);
	return convertToBaseUnit(mvalue, mexp);
}
MathStructure AliasUnit::convertFromBaseUnit() const {
	MathStructure mexp(1, 1);
	MathStructure mvalue(1, 1);
	return convertFromBaseUnit(mvalue, mexp);
}

int AliasUnit::baseExponent(int exp) const {
	return o_unit->baseExponent(exp * i_exp);
}

MathStructure &AliasUnit::convertFromFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	if(i_exp != 1) mexp /= i_exp;
	ParseOptions po;
	if(isApproximate() && suncertainty.empty() && precision() == -1) {
		if(sinverse.find(DOT) != string::npos || svalue.find(DOT) != string::npos) po.read_precision = READ_PRECISION_WHEN_DECIMALS;
		else po.read_precision = ALWAYS_READ_PRECISION;
	}
	if(sinverse.empty()) {
		if(svalue.find("\\x") != string::npos) {
			string stmp = svalue;
			string stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
			int x_id = CALCULATOR->addId(new MathStructure(mvalue), true);
			stmp2 += i2s(x_id);
			stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
			gsub("\\x", stmp2, stmp);
			stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
			int y_id = -1;
			if(svalue.find("\\y") != string::npos) {
				stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
				y_id = CALCULATOR->addId(new MathStructure(mexp), true);
				stmp2 += i2s(y_id);
				stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				gsub("\\y", stmp2, stmp);
			}
			CALCULATOR->parse(&mvalue, stmp, po);
			CALCULATOR->delId(x_id);
			if(y_id < 0) {
				if(!mexp.isOne()) mvalue.raise(mexp);
			} else {
				CALCULATOR->delId(y_id);
			}
			if(precision() > 0 && (mvalue.precision() < 0 || precision() < mvalue.precision())) mvalue.setPrecision(precision(), true);
			if(isApproximate()) mvalue.setApproximate(true, true);
		} else {
			MathStructure *mstruct = new MathStructure();
			bool b_number = false;
			if(!suncertainty.empty()) {
				b_number = true;
			} else {
				size_t i = svalue.rfind(')');
				if(i != string::npos && i > 2 && (i == svalue.length() - 1 || (i < svalue.length() - 2 && (svalue[i + 1] == 'E' || svalue[i + 1] == 'e')))) {
					size_t i2 = svalue.rfind('(');
					if(i2 != string::npos && i2 < i - 1) {
						if(svalue.find_first_not_of(NUMBER_ELEMENTS SPACES, svalue[0] == '-' || svalue[0] == '+' ? 1 : 0) == i2 && svalue.find_first_not_of(NUMBERS SPACES, i2 + 1) == i && (i == svalue.length() - 1 || svalue.find_first_not_of(NUMBER_ELEMENTS SPACES, svalue[i + 2] == '-' || svalue[i + 2] == '+' ? i + 3 : i + 2) == string::npos)) {
							b_number = true;
						}
					}
				}
			}
			if(b_number) {
				mstruct->number().set(svalue, po);
				mstruct->numberUpdated();
			} else {
				CALCULATOR->parse(mstruct, svalue, po);
				if(mstruct->containsType(STRUCT_UNIT, false, true, true)) {
					mstruct->transformById(FUNCTION_ID_STRIP_UNITS);
				}
			}
			if(!suncertainty.empty()) {
				Number nr_u(suncertainty);
				if(mstruct->isNumber()) {
					if(b_relative_uncertainty) mstruct->number().setRelativeUncertainty(nr_u);
					else mstruct->number().setUncertainty(nr_u);
					mstruct->numberUpdated();
				} else if(mstruct->isMultiplication() && mstruct->size() > 0 && (*mstruct)[0].isNumber()) {
					if(b_relative_uncertainty) (*mstruct)[0].number().setRelativeUncertainty(nr_u);
					else (*mstruct)[0].number().setUncertainty(nr_u);
					(*mstruct)[0].numberUpdated();
					mstruct->childUpdated(1);
				}
			} else if(precision() > 0) {
				if(mstruct->isNumber()) {
					if(mstruct->number().precision() < 1 || precision() < mstruct->number().precision()) {
						mstruct->number().setPrecision(precision());
						mstruct->numberUpdated();
					}
				} else if(mstruct->isMultiplication() && mstruct->getChild(1)->isNumber()) {
					if(mstruct->getChild(1)->number().precision() < 0 || precision() < mstruct->getChild(1)->number().precision()) {
						mstruct->getChild(1)->number().setPrecision(precision());
						mstruct->getChild(1)->numberUpdated();
						mstruct->childUpdated(1);
					}
				} else if(mstruct->precision() < 0 || precision() < mstruct->precision()) {
					mstruct->setPrecision(precision(), true);
				}
			} else if(isApproximate() && !mstruct->isApproximate()) {
				mstruct->setApproximate(true, true);
			}
			if(!mexp.isOne()) mstruct->raise(mexp);
			mvalue.divide_nocopy(mstruct, true);
		}
	} else {
		if(sinverse.find("\\x") != string::npos) {
			string stmp = sinverse;
			string stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
			int x_id = CALCULATOR->addId(new MathStructure(mvalue), true);
			stmp2 += i2s(x_id);
			stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
			gsub("\\x", stmp2, stmp);
			int y_id = -1;
			if(svalue.find("\\y") != string::npos) {
				stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
				y_id = CALCULATOR->addId(new MathStructure(mexp), true);
				stmp2 += i2s(y_id);
				stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				gsub("\\y", stmp2, stmp);
			}
			CALCULATOR->parse(&mvalue, stmp, po);
			CALCULATOR->delId(x_id);
			if(y_id < 0) {
				if(!mexp.isOne()) mvalue.raise(mexp);
			} else {
				CALCULATOR->delId(y_id);
			}
			if(precision() > 0 && (mvalue.precision() < 0|| precision() < mvalue.precision())) mvalue.setPrecision(precision(), true);
			if(isApproximate()) mvalue.setApproximate(true, true);
		} else {
			MathStructure *mstruct = new MathStructure();
			CALCULATOR->parse(mstruct, sinverse, po);
			if(precision() > 0) {
				if(mstruct->isNumber()) {
					if(mstruct->number().precision() < 0 || precision() < mstruct->number().precision()) {
						mstruct->number().setPrecision(precision());
						mstruct->numberUpdated();
					}
				} else if(mstruct->isMultiplication() && mstruct->getChild(1)->isNumber()) {
					if(mstruct->getChild(1)->number().precision() < 0 || precision() < mstruct->getChild(1)->number().precision()) {
						mstruct->getChild(1)->number().setPrecision(precision());
						mstruct->getChild(1)->numberUpdated();
						mstruct->childUpdated(1);
					}
				} else if(mstruct->precision() < 0 || precision() < mstruct->precision()) {
					mstruct->setPrecision(precision(), true);
				}
			} else if(isApproximate() && !mstruct->isApproximate()) {
				mstruct->setApproximate(true, true);
			}
			if(!mexp.isOne()) mstruct->raise(mexp);
			if(mvalue.isOne()) mvalue.set_nocopy(*mstruct);
			else mvalue.multiply_nocopy(mstruct, true);
		}
	}
	return mvalue;
}
MathStructure &AliasUnit::convertToFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	ParseOptions po;
	if(isApproximate() && suncertainty.empty() && precision() == -1) {
		if(svalue.find(DOT) != string::npos) po.read_precision = READ_PRECISION_WHEN_DECIMALS;
		else po.read_precision = ALWAYS_READ_PRECISION;
	}
	if(svalue.find("\\x") != string::npos) {
		string stmp = svalue;
		string stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
		int x_id = CALCULATOR->addId(new MathStructure(mvalue), true);
		int y_id = -1;
		stmp2 += i2s(x_id);
		stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
		gsub("\\x", stmp2, stmp);
		if(svalue.find("\\y") != string::npos) {
			stmp2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
			y_id = CALCULATOR->addId(new MathStructure(mexp), true);
			stmp2 += i2s(y_id);
			stmp2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
			gsub("\\y", stmp2, stmp);
		}
		CALCULATOR->parse(&mvalue, stmp, po);
		CALCULATOR->delId(x_id);
		if(y_id < 0) {
			if(!mexp.isOne()) mvalue.raise(mexp);
		} else {
			CALCULATOR->delId(y_id);
		}
		if(precision() >= 0 && (mvalue.precision() < 0 || precision() < mvalue.precision())) mvalue.setPrecision(precision(), true);
		if(isApproximate()) mvalue.setApproximate(true, true);
	} else {
		MathStructure *mstruct = new MathStructure();
		bool b_number = false;
		if(!suncertainty.empty()) {
			b_number = true;
		} else {
			size_t i = svalue.rfind(')');
			if(i != string::npos && i > 2 && (i == svalue.length() - 1 || (i < svalue.length() - 2 && (svalue[i + 1] == 'E' || svalue[i + 1] == 'e')))) {
				size_t i2 = svalue.rfind('(');
				if(i2 != string::npos && i2 < i - 1) {
					if(svalue.find_first_not_of(NUMBER_ELEMENTS SPACES, svalue[0] == '-' || svalue[0] == '+' ? 1 : 0) == i2 && svalue.find_first_not_of(NUMBERS SPACES, i2 + 1) == i && (i == svalue.length() - 1 || svalue.find_first_not_of(NUMBER_ELEMENTS SPACES, svalue[i + 2] == '-' || svalue[i + 2] == '+' ? i + 3 : i + 2) == string::npos)) {
						b_number = true;
					}
				}
			}
		}
		if(b_number) {
			mstruct->number().set(svalue, po);
			mstruct->numberUpdated();
		} else {
			CALCULATOR->parse(mstruct, svalue, po);
			if(mstruct->containsType(STRUCT_UNIT, false, true, true) > 0) {
				mstruct->transformById(FUNCTION_ID_STRIP_UNITS);
			}
		}
		if(!suncertainty.empty()) {
			Number nr_u(suncertainty);
			if(mstruct->isNumber()) {
				if(b_relative_uncertainty) mstruct->number().setRelativeUncertainty(nr_u);
				else mstruct->number().setUncertainty(nr_u);
				mstruct->numberUpdated();
			} else if(mstruct->isMultiplication() && mstruct->size() > 0 && (*mstruct)[0].isNumber()) {
				if(b_relative_uncertainty) (*mstruct)[0].number().setRelativeUncertainty(nr_u);
				else (*mstruct)[0].number().setUncertainty(nr_u);
				(*mstruct)[0].numberUpdated();
				mstruct->childUpdated(1);
			}
		} else if(precision() >= 0) {
			if(mstruct->isNumber()) {
				if(mstruct->number().precision() < 0 || precision() < mstruct->number().precision()) {
					mstruct->number().setPrecision(precision());
					mstruct->numberUpdated();
				}
			} else if(mstruct->isMultiplication() && mstruct->getChild(1)->isNumber()) {
				if(mstruct->getChild(1)->number().precision() < 0 || precision() < mstruct->getChild(1)->number().precision()) {
					mstruct->getChild(1)->number().setPrecision(precision());
					mstruct->getChild(1)->numberUpdated();
					mstruct->childUpdated(1);
				}
			} else if(mstruct->precision() < 0 || precision() < mstruct->precision()) {
				mstruct->setPrecision(precision(), true);
			}
		} else if(isApproximate() && !mstruct->isApproximate()) {
			mstruct->setApproximate(true, true);
		}
		if(!mexp.isOne()) mstruct->raise(mexp);
		if(mvalue.isOne()) mvalue.set_nocopy(*mstruct);
		else mvalue.multiply_nocopy(mstruct, true);
	}
	if(i_exp != 1) mexp.multiply(i_exp);
	return mvalue;
}
void AliasUnit::setExponent(int exp) {
	i_exp = exp;
	setChanged(true);
}
int AliasUnit::firstBaseExponent() const {
	return i_exp;
}
int AliasUnit::mixWithBase() const {return i_mix;}
int AliasUnit::mixWithBaseMinimum() const {return i_mix_min;}
void AliasUnit::setMixWithBase(int mix_priority) {i_mix = mix_priority;}
void AliasUnit::setMixWithBaseMinimum(int mix_minimum) {i_mix_min = mix_minimum;}
int AliasUnit::subtype() const {
	return SUBTYPE_ALIAS_UNIT;
}
bool AliasUnit::isChildOf(Unit *u) const {
	if(u == this) return false;
	if(baseUnit() == u) return true;
	if(u->baseUnit() != baseUnit()) return false;
	Unit *u2 = (Unit*) this;
	while(1) {
		u2 = (Unit*) ((AliasUnit*) u2)->firstBaseUnit();
		if(u == u2) return true;
		if(u2->subtype() != SUBTYPE_ALIAS_UNIT) return false;
	}
	return false;
}
bool AliasUnit::isParentOf(Unit *u) const {
	if(u == this) return false;
	if(u->baseUnit() != baseUnit()) return false;
	while(1) {
		if(u->subtype() != SUBTYPE_ALIAS_UNIT) return false;
		u = ((AliasUnit*) u)->firstBaseUnit();
		if(u == this) return true;
	}
	return false;
}
bool AliasUnit::hasNonlinearExpression() const {
	return svalue.find("\\x") != string::npos;
}
bool AliasUnit::hasNonlinearRelationTo(Unit *u) const {
	if(u == this) return false;
	Unit *ub = baseUnit();
	Unit *ub2 = u->baseUnit();
	if(ub2 != ub) {
		if(ub->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(hasNonlinearRelationTo(ub)) return ((CompositeUnit*) ub)->containsRelativeTo(u);
			for(size_t i = 1; i <= ((CompositeUnit*) ub)->countUnits(); i++) {
				if(((CompositeUnit*) ub)->get(i)->hasNonlinearRelationTo(u)) return true;
			}
			return false;
		}
		if(ub2->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) ub2)->containsRelativeTo(baseUnit()) && (u->hasNonlinearRelationTo(ub2) || hasNonlinearRelationTo(ub))) return true;
		}
		return false;
	}
	if(isParentOf(u)) {
		Unit *fbu = u;
		while(true) {
			if((const Unit*) fbu == this) return false;
			if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
			if(((AliasUnit*) fbu)->hasNonlinearExpression()) return true;
			fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
		}
	} else if(isChildOf(u)) {
		Unit *fbu = (Unit*) this;
		while(true) {
			if((const Unit*) fbu == u) return false;
			if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
			if(((AliasUnit*) fbu)->hasNonlinearExpression()) return true;
			fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
		}
	} else {
		return hasNonlinearRelationTo(baseUnit()) || u->hasNonlinearRelationTo(u->baseUnit());
	}
}
bool AliasUnit::hasApproximateExpression(bool check_variables, bool ignore_high_precision_intervals) const {
	if(isApproximate()) return true;
	if(svalue.find_first_not_of(NUMBER_ELEMENTS EXPS) == string::npos) return false;
	MathStructure m(1, 1, 0), mexp(1, 1, 0);
	convertToFirstBaseUnit(m, mexp);
	return m.containsInterval(true, check_variables, false, ignore_high_precision_intervals ? 1 : 0, true);
}
bool AliasUnit::hasApproximateRelationTo(Unit *u, bool check_variables, bool ignore_high_precision_intervals) const {
	if(u == this) return false;
	Unit *ub = baseUnit();
	Unit *ub2 = u->baseUnit();
	if(ub2 != ub) {
		if(ub->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) ub)->containsRelativeTo(u) && hasApproximateRelationTo(ub, check_variables, ignore_high_precision_intervals)) return true;
			for(size_t i = 1; i <= ((CompositeUnit*) ub)->countUnits(); i++) {
				if(((CompositeUnit*) ub)->get(i)->hasApproximateRelationTo(u, check_variables, ignore_high_precision_intervals)) return true;
			}
			return false;
		}
		if(ub2->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) ub2)->containsRelativeTo(baseUnit()) && (u->hasApproximateRelationTo(ub2, check_variables, ignore_high_precision_intervals) || hasApproximateRelationTo(ub, check_variables, ignore_high_precision_intervals))) return true;
		}
		return false;
	}
	if(isParentOf(u)) {
		Unit *fbu = u;
		if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
		while(true) {
			if((const Unit*) fbu == this) return false;
			if(((AliasUnit*) fbu)->hasApproximateExpression(check_variables)) return true;
			if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
			fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
		}
	} else if(isChildOf(u)) {
		Unit *fbu = (Unit*) this;
		if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
		while(true) {
			if((const Unit*) fbu == u) return false;
			if(((AliasUnit*) fbu)->hasApproximateExpression(check_variables)) return true;
			if(fbu->subtype() != SUBTYPE_ALIAS_UNIT) return false;
			fbu = (Unit*) ((AliasUnit*) fbu)->firstBaseUnit();
		}
	} else {
		return hasApproximateRelationTo(baseUnit(), check_variables, ignore_high_precision_intervals) || u->hasApproximateRelationTo(u->baseUnit(), check_variables, ignore_high_precision_intervals);
	}
}
bool AliasUnit::containsRelativeTo(Unit *u) const {
	if(!u || u == this) return false;
	return baseUnit() == u->baseUnit() || baseUnit()->containsRelativeTo(u->baseUnit());
}
bool AliasUnit::hasNonlinearRelationToBase() const {return hasNonlinearRelationTo(baseUnit()) || baseUnit()->hasNonlinearRelationToBase();}
bool AliasUnit::hasApproximateRelationToBase(bool check_variables, bool ignore_high_precision_intervals) const {return hasApproximateRelationTo(baseUnit(), check_variables, ignore_high_precision_intervals) || baseUnit()->hasApproximateRelationToBase(check_variables, ignore_high_precision_intervals);}

AliasUnit_Composite::AliasUnit_Composite(Unit *alias, int exp, Prefix *prefix_) : AliasUnit("", alias->referenceName(), "", "", "", alias, "", exp, "") {
	prefixv = (Prefix*) prefix_;
}
AliasUnit_Composite::AliasUnit_Composite(const AliasUnit_Composite *unit) {
	set(unit);
}
AliasUnit_Composite::~AliasUnit_Composite() {}
ExpressionItem *AliasUnit_Composite::copy() const {
	return new AliasUnit_Composite(this);
}
void AliasUnit_Composite::set(const ExpressionItem *item) {
	if(item->type() == TYPE_UNIT) {
		if(((Unit*) item)->subtype() == SUBTYPE_ALIAS_UNIT) {
			AliasUnit::set(item);
			prefixv = (Prefix*) ((AliasUnit_Composite*) item)->prefix();
		} else {
			Unit::set(item);
		}
	} else {
		ExpressionItem::set(item);
	}
}
string AliasUnit_Composite::print(const PrintOptions &po, bool format, int tagtype, bool input, bool plural) const {
	string str = "";
	const ExpressionName *ename;
	if(input) {
		ename = &o_unit->preferredInputName(po.abbreviate_names, po.use_unicode_signs, plural, po.use_reference_names || (po.preserve_format && o_unit->isCurrency()), po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
		if(prefixv) str = prefixv->preferredInputName(ename->abbreviation, po.use_unicode_signs, plural, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name;
	} else {
		ename = &o_unit->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, plural, po.use_reference_names || (po.preserve_format && o_unit->isCurrency()), po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
		if(prefixv) str = prefixv->preferredDisplayName(ename->abbreviation, po.use_unicode_signs, plural, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name;
	}
	str += ename->formattedName(TYPE_UNIT, !po.use_reference_names && tagtype != TAG_TYPE_TERMINAL, format && tagtype == TAG_TYPE_HTML, !po.use_reference_names && !po.preserve_format, po.hide_underscore_spaces);
	return str;
}
string AliasUnit_Composite::print(bool plural_, bool short_, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	string str = "";
	const ExpressionName *ename = &o_unit->preferredName(short_, use_unicode, plural_, false, can_display_unicode_string_function, can_display_unicode_string_arg);
	if(prefixv) {
		str += prefixv->preferredName(ename->abbreviation, use_unicode, plural_, false, can_display_unicode_string_function, can_display_unicode_string_arg).name;
	}
	str += ename->name;
	return str;
}
Prefix *AliasUnit_Composite::prefix() const {
	return prefixv;
}
int AliasUnit_Composite::prefixExponent() const {
	if(prefixv && prefixv->type() == PREFIX_DECIMAL) return ((DecimalPrefix*) prefixv)->exponent();
	if(prefixv && prefixv->type() == PREFIX_BINARY) return ((BinaryPrefix*) prefixv)->exponent();
	return 0;
}
void AliasUnit_Composite::set(Unit *u, int exp, Prefix *prefix_) {
	setBaseUnit(u);
	setExponent(exp);
	prefixv = (Prefix*) prefix_;
}
MathStructure &AliasUnit_Composite::convertToFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	if(prefixv) {
		MathStructure *mstruct = new MathStructure(prefixv->value());
		if(!mexp.isOne()) mstruct->raise(mexp);
		mvalue.multiply_nocopy(mstruct, true);
	}
	if(i_exp != 1) mexp.multiply(i_exp);
	return mvalue;
}
MathStructure &AliasUnit_Composite::convertFromFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const {
	if(i_exp != 1) mexp /= i_exp;
	if(prefixv) {
		MathStructure *mstruct = new MathStructure(prefixv->value());
		if(!mexp.isOne()) mstruct->raise(mexp);
		mvalue.divide_nocopy(mstruct, true);
	}
	return mvalue;
}

CompositeUnit::CompositeUnit(string cat_, string name_, string title_, string base_expression_, bool is_local, bool is_builtin, bool is_active) : Unit(cat_, name_, "", "", title_, is_local, is_builtin, is_active) {
	setBaseExpression(base_expression_);
	setChanged(false);
}
CompositeUnit::CompositeUnit(const CompositeUnit *unit) {
	set(unit);
}
CompositeUnit::~CompositeUnit() {
	clear();
}
ExpressionItem *CompositeUnit::copy() const {
	return new CompositeUnit(this);
}
void CompositeUnit::set(const ExpressionItem *item) {
	if(item->type() == TYPE_UNIT) {
		Unit::set(item);
		if(((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			CompositeUnit *cu = (CompositeUnit*) item;
			for(size_t i = 1; i <= cu->countUnits(); i++) {
				int exp = 1; Prefix *p = NULL;
				Unit *u = cu->get(i, &exp, &p);
				units.push_back(new AliasUnit_Composite(u, exp, p));
			}
		}
	} else {
		ExpressionItem::set(item);
	}
}
void CompositeUnit::add(Unit *u, int exp, Prefix *prefix) {
	bool b = false;
	for(size_t i = 0; i < units.size(); i++) {
		if(exp > units[i]->firstBaseExponent()) {
			units.insert(units.begin() + i, new AliasUnit_Composite(u, exp, prefix));
			b = true;
			break;
		}
	}
	if(!b) {
		units.push_back(new AliasUnit_Composite(u, exp, prefix));
	}
}
Unit *CompositeUnit::get(size_t index, int *exp, Prefix **prefix) const {
	if(index > 0 && index <= units.size()) {
		if(exp) *exp = units[index - 1]->firstBaseExponent();
		if(prefix) *prefix = (Prefix*) units[index - 1]->prefix();
		return (Unit*) units[index - 1]->firstBaseUnit();
	}
	return NULL;
}
void CompositeUnit::setExponent(size_t index, int exp) {
	if(index > 0 && index <= units.size()) {
		bool b = exp > units[index - 1]->firstBaseExponent();
		units[index - 1]->setExponent(exp);
		if(b) {
			for(size_t i = 0; i < index - 1; i++) {
				if(exp > units[i]->firstBaseExponent()) {
					AliasUnit_Composite *u = units[index - 1];
					units.erase(units.begin() + (index - 1));
					units.insert(units.begin() + i, u);
					break;
				}
			}
		} else {
			for(size_t i = units.size() - 1; i > index - 1; i--) {
				if(exp < units[i]->firstBaseExponent()) {
					AliasUnit_Composite *u = units[index - 1];
					units.insert(units.begin() + i, u);
					units.erase(units.begin() + (index - 1));
					break;
				}
			}
		}
	}
}
void CompositeUnit::setPrefix(size_t index, Prefix *prefix) {
	if(index > 0 && index <= units.size()) {
		units[index - 1]->set(units[index - 1]->firstBaseUnit(), units[index - 1]->firstBaseExponent(), prefix);
	}
}
size_t CompositeUnit::countUnits() const {
	return units.size();
}
size_t CompositeUnit::find(Unit *u) const {
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->firstBaseUnit() == u) {
			return i + 1;
		}
	}
	return 0;
}
void CompositeUnit::del(size_t index) {
	if(index > 0 && index <= units.size()) {
		delete units[index - 1];
		units.erase(units.begin() + (index - 1));
	}
}
string CompositeUnit::print(const PrintOptions &po, bool format, int tagtype, bool input, bool plural) const {
	string str = "";
	bool b = false, b2 = false;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->firstBaseExponent() != 0) {
			if(!b && units[i]->firstBaseExponent() < 0 && i > 0) {
				if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
					str += SIGN_DIVISION;
				} else if(!input && po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
					str += " " SIGN_DIVISION_SLASH " ";
				} else {
					str += "/";
				}
				b = true;
				if(i < units.size() - 1) {
					b2 = true;
					str += "(";
				}
			} else {
				if(i > 0) {
					if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
					else if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
					else str += " ";
				}
			}
			str += units[i]->print(po, format, tagtype, input, plural && i == 0 && units[i]->firstBaseExponent() > 0);
			if(po.abbreviate_names && po.use_unicode_signs && units[i]->firstBaseExponent() != (b ? -1 : 1) && str.length() >= 2 && str[str.length() - 1] == (char) -80 && str[str.length() - 2] == (char) -62) {
				str.erase(str.length() - 2, 2);
				str += units[i]->print(po, format, tagtype, input);
			}
			if(b) {
				if(units[i]->firstBaseExponent() != -1) {
					if(format && tagtype == TAG_TYPE_HTML) {
						str += "<sup>";
						str += i2s(-units[i]->firstBaseExponent());
						str += "</sup>";
					} else if(po.use_unicode_signs && units[i]->firstBaseExponent() == -2 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_POWER_2, po.can_display_unicode_string_arg))) str += SIGN_POWER_2;
					else if(po.use_unicode_signs && units[i]->firstBaseExponent() == -3 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_POWER_3, po.can_display_unicode_string_arg))) str += SIGN_POWER_3;
					else {
						str += "^";
						str += i2s(-units[i]->firstBaseExponent());
					}
				}
			} else {
				if(units[i]->firstBaseExponent() != 1) {
					if(format && tagtype == TAG_TYPE_HTML) {
						str += "<sup>";
						if(units[i]->firstBaseExponent() < 0 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
							str += SIGN_MINUS;
							str += i2s(-units[i]->firstBaseExponent());
						} else {
							str += i2s(units[i]->firstBaseExponent());
						}
						str += "</sup>";
					} else if(po.use_unicode_signs && units[i]->firstBaseExponent() == 2 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_POWER_2, po.can_display_unicode_string_arg))) str += SIGN_POWER_2;
					else if(po.use_unicode_signs && units[i]->firstBaseExponent() == 3 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_POWER_3, po.can_display_unicode_string_arg))) str += SIGN_POWER_3;
					else {
						str += "^";
						if(units[i]->firstBaseExponent() < 0 && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) {
							str += SIGN_MINUS;
							str += i2s(-units[i]->firstBaseExponent());
						} else {
							str += i2s(units[i]->firstBaseExponent());
						}
					}
				}
			}
		}
	}
	if(b2) str += ")";
	return str;
}
string CompositeUnit::print(bool plural_, bool short_, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	string str = "";
	bool b = false, b2 = false;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->firstBaseExponent() != 0) {
			if(!b && units[i]->firstBaseExponent() < 0 && i > 0) {
				str += "/";
				b = true;
				if(i < units.size() - 1) {
					b2 = true;
					str += "(";
				}
			} else {
				if(i > 0) {
					if(use_unicode && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_MIDDLEDOT, can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
					else str += " ";
				}
			}
			if(plural_ && i == 0 && units[i]->firstBaseExponent() > 0) {
				str += units[i]->print(true, short_, use_unicode, can_display_unicode_string_function, can_display_unicode_string_arg);
			} else {
				str += units[i]->print(false, short_, use_unicode, can_display_unicode_string_function, can_display_unicode_string_arg);
			}
			if(short_ && use_unicode && units[i]->firstBaseExponent() != (b ? -1 : 1) && str.length() >= 2 && str[str.length() - 1] == (char) -80 && str[str.length() - 2] == (char) -62) {
				str.erase(str.length() - 2, 2);
				str += units[i]->print(plural_ && i == 0 && units[i]->firstBaseExponent() > 0, short_, false, can_display_unicode_string_function, can_display_unicode_string_arg);
			}
			if(b) {
				if(units[i]->firstBaseExponent() != -1) {
					if(use_unicode && units[i]->firstBaseExponent() == -2 && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_POWER_2, can_display_unicode_string_arg))) str += SIGN_POWER_2;
					else if(use_unicode && units[i]->firstBaseExponent() == -3 && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_POWER_3, can_display_unicode_string_arg))) str += SIGN_POWER_3;
					else {
						str += "^";
						str += i2s(-units[i]->firstBaseExponent());
					}
				}
			} else {
				if(units[i]->firstBaseExponent() != 1) {
					if(use_unicode && units[i]->firstBaseExponent() == 2 && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_POWER_2, can_display_unicode_string_arg))) str += SIGN_POWER_2;
					else if(use_unicode && units[i]->firstBaseExponent() == 3 && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_POWER_3, can_display_unicode_string_arg))) str += SIGN_POWER_3;
					else {
						str += "^";
						if(units[i]->firstBaseExponent() < 0 && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (SIGN_MINUS, can_display_unicode_string_arg))) {
							str += SIGN_MINUS;
							str += i2s(-units[i]->firstBaseExponent());
						} else {
							str += i2s(units[i]->firstBaseExponent());
						}
					}
				}
			}
		}
	}
	if(b2) str += ")";
	return str;
}
int CompositeUnit::subtype() const {
	return SUBTYPE_COMPOSITE_UNIT;
}
bool CompositeUnit::containsRelativeTo(Unit *u) const {
	if(!u || u == this) return false;
	CompositeUnit *cu;
	for(size_t i = 0; i < units.size(); i++) {
		if(u == units[i] || u->baseUnit() == units[i]->baseUnit()) return true;
		if(units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			cu = (CompositeUnit*) units[i]->baseUnit();
			if(cu->containsRelativeTo(u)) return true;
		}
	}
	if(u->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		cu = (CompositeUnit*) u->baseUnit();
		for(size_t i = 1; i <= cu->countUnits(); i++) {
			if(containsRelativeTo(cu->get(i)->baseUnit())) return true;
		}
		return false;
	}
	return false;
}
bool CompositeUnit::hasNonlinearRelationToBase() const {
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->hasNonlinearRelationToBase()) return true;
	}
	return false;
}
bool CompositeUnit::hasApproximateRelationToBase(bool check_variables, bool ignore_high_precision_intervals) const {
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->hasApproximateRelationToBase(check_variables, ignore_high_precision_intervals)) return true;
	}
	return false;
}
MathStructure CompositeUnit::generateMathStructure(bool make_division, bool set_null_prefixes) const {
	MathStructure mstruct;
	bool has_p = set_null_prefixes;
	if(!set_null_prefixes) {
		for(size_t i = 0; i < units.size(); i++) {
			if(units[i]->prefix()) {
				has_p = true;
				break;
			}
		}
	}
	MathStructure mden;
	for(size_t i = 0; i < units.size(); i++) {
		MathStructure mstruct2;
		if(!has_p || units[i]->prefix()) {
			mstruct2.set(units[i]->firstBaseUnit(), units[i]->prefix());
		} else {
			mstruct2.set(units[i]->firstBaseUnit(), CALCULATOR->getDecimalNullPrefix());
		}
		if(make_division && units[i]->firstBaseExponent() < 0) {
			if(units[i]->firstBaseExponent() != -1) {
				mstruct2 ^= -units[i]->firstBaseExponent();
			}
		} else if(units[i]->firstBaseExponent() != 1) {
			mstruct2 ^= units[i]->firstBaseExponent();
		}
		if(i == 0) {
			if(make_division && units[i]->firstBaseExponent() < 0) {
				mstruct = 1;
				mden = mstruct2;
			} else {
				mstruct = mstruct2;
			}
		} else if(make_division && units[i]->firstBaseExponent() < 0) {
			if(mden.isZero()) {
				mden = mstruct2;
			} else {
				mden.multiply(mstruct2, true);
			}
		} else {
			mstruct.multiply(mstruct2, true);
		}
	}
	if(make_division && !mden.isZero()) {
		mstruct.transform(STRUCT_DIVISION, mden);
	}
	return mstruct;
}
void remove_times_one(MathStructure &m) {
	if(m.isMultiplication() && m.size() >= 2) {
		for(size_t i = 0; i < m.size();) {
			remove_times_one(m[i]);
			if(m[i].isOne()) {
				m.delChild(i + 1);
				if(m.size() == 1) {
					m.setToChild(1, true);
					break;
				}
			} else {
				i++;
			}
		}
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			remove_times_one(m[i]);
		}
	}
}
bool fix_division(MathStructure &m, const EvaluationOptions &eo) {
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_division(m[i], eo)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(m.isPower() && !m[0].isUnit()) {
		if(m.calculatesub(eo, eo, false)) b_ret = true;
	}
	return b_ret;
}
bool replace_variables(MathStructure &m) {
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_variables(m[i])) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		Unit *u = CALCULATOR->getActiveUnit(m.variable()->referenceName() + "_unit");
		if(!u) {
			if(m.variable()->referenceName() == "bohr_radius") u = CALCULATOR->getActiveUnit("bohr_unit");
			else if(m.variable()->referenceName() == "elementary_charge") u = CALCULATOR->getActiveUnit("e_unit");
			else if(m.variable()->referenceName() == "electron_mass") u = CALCULATOR->getActiveUnit("electron_unit");
		}
		if(u) {
			m.set(u, true);
			b_ret = true;
		}
	}
	return b_ret;
}
void CompositeUnit::setBaseExpression(string base_expression_) {
	clear();
	if(base_expression_.empty()) {
		setChanged(true);
		return;
	}
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_EXACT;
	eo.sync_units = false;
	eo.keep_prefixes = true;
	eo.structuring = STRUCTURING_NONE;
	eo.reduce_divisions = false;
	eo.do_polynomial_division = false;
	eo.isolate_x = false;
	ParseOptions po;
	bool conversion_variant = !name().empty() && name()[0] == '0';
	po.variables_enabled = true;
	po.functions_enabled = conversion_variant;
	po.unknowns_enabled = !conversion_variant;
	if(name().length() >= 2 && name()[1] == '1') po.limit_implicit_multiplication = true;
	MathStructure mstruct;
	bool had_errors = false;
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->parse(&mstruct, base_expression_, po);
	replace_variables(mstruct);
	if(!conversion_variant && mstruct.containsType(STRUCT_VARIABLE, true)) {
		po.variables_enabled = false;
		CALCULATOR->parse(&mstruct, base_expression_, po);
	}
	remove_times_one(mstruct);
	fix_division(mstruct, eo);
	bool b_eval = !is_unit_multiexp(mstruct);
	while(true) {
		if(b_eval) mstruct.eval(eo);
		if(mstruct.isUnit()) {
			add(mstruct.unit(), 1, mstruct.prefix());
		} else if(mstruct.isPower() && mstruct[0].isUnit() && mstruct[1].isInteger()) {
			add(mstruct[0].unit(), mstruct[1].number().intValue(), mstruct[0].prefix());
		} else if(mstruct.isMultiplication()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isUnit()) {
					add(mstruct[i].unit(), 1, mstruct[i].prefix());
				} else if(mstruct[i].isPower() && mstruct[i][0].isUnit() && mstruct[i][1].isInteger()) {
					add(mstruct[i][0].unit(), mstruct[i][1].number().intValue(), mstruct[i][0].prefix());
				} else if(mstruct[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
						if(mstruct[i][i2].isUnit()) {
							add(mstruct[i][i2].unit(), 1, mstruct[i][i2].prefix());
						} else if(mstruct[i][i2].isPower() && mstruct[i][i2][0].isUnit() && mstruct[i][i2][1].isInteger()) {
							add(mstruct[i][i2][0].unit(), mstruct[i][i2][1].number().intValue(), mstruct[i][i2][0].prefix());
						} else {
							had_errors = true;
						}
					}
				} else {
					had_errors = true;
				}
			}
		} else {
			had_errors = true;
		}
		if(had_errors && !b_eval) {
			had_errors = false;
			b_eval = true;
			clear();
		} else {
			break;
		}
	}
	if(conversion_variant && had_errors) {
		CALCULATOR->endTemporaryStopMessages();
		CALCULATOR->error(true, _("Error(s) in unitexpression."), NULL);
	} else {
		if(CALCULATOR->endTemporaryStopMessages() > 0) had_errors = true;
		if(had_errors) CALCULATOR->error(false, _("Error(s) in unitexpression."), NULL);
	}
	setChanged(true);
}
void CompositeUnit::clear() {
	for(size_t i = 0; i < units.size(); i++) {
		delete units[i];
	}
	units.clear();
}
