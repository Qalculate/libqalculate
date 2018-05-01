/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNIT_H
#define UNIT_H

/** @file */

/// Type of unit
typedef enum {
	/// class Unit
	SUBTYPE_BASE_UNIT,
	/// class AliasUnit
	SUBTYPE_ALIAS_UNIT,
	/// class CompositeUnit
	SUBTYPE_COMPOSITE_UNIT
} UnitSubtype;

#include <libqalculate/ExpressionItem.h>
#include <libqalculate/includes.h>

/// A unit for measurement.
/**
* The Unit class both represents a base unit and is the base class for other unit types.
* Base units are units defined as basis for other units. Meters and seconds are typical base units.
*
* For base units, a name is all that is needed.
* Base units do however normally have three different names defined for use in expressions - abbreviation (ex. "m"), singular ("meter") and plural ("meters").
*/
class Unit : public ExpressionItem {

  protected:

	string ssystem, scountries;
	bool b_si;
	bool b_use_with_prefixes;

  public:

	Unit(string cat_, string name_, string plural_ = "", string singular_ = "", string title_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	Unit();	
	Unit(const Unit *unit);	
	virtual ~Unit();

	virtual ExpressionItem *copy() const;
	virtual void set(const ExpressionItem *item);

	/** Returns if the unit is part of the SI standard.
	*
	* @returns true if the unit is part of the SI standard.
	*/
	bool isSIUnit() const;
	/** State that the unit is part of the SI standard.
	* Sets system to "SI".
	*/
	void setAsSIUnit();
	/** Sets which system/standard ("SI", "CGS", etc.) the unit is part of.
	* Setting system to "SI" (case-insensitive), is equivalent to setAsSIUnit().
	*/
	void setSystem(string s_system);
	/** Returns the system/standard that the unit is part of.
	*
	* @returns System string.
	*/
	const string &system() const;	
	/** Returns wether prefixes should be used with this unit or not.
	*
	* @returns true if the prefixes is appropriate for this unit.
	*/
	bool useWithPrefixesByDefault() const;
	/** Sets wether prefixes are approriate with this unit or not.
	*/
	void setUseWithPrefixesByDefault(bool use_with_prefixes);
	/** Returns if the unit is a currency (Euro is base unit).
	*
	* @returns true if the unit is a currency.
	*/
	bool isCurrency() const;
	const string &countries() const;
	void setCountries(string country_names);
	/** Returns a display string representing the unit in an expression.
	*
	* Equivalent to preferredName() for Unit and AliasUnit, but closer to MathStructure::print() for CompositeUnit (prints out base expression).
	*/
	virtual string print(bool plural_, bool short_, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual const string &plural(bool return_singular_if_no_plural = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual const string &singular(bool return_abbreviation_if_no_singular = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual const string &abbreviation(bool return_singular_if_no_abbreviation = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual bool isUsedByOtherUnits() const;
	virtual Unit* baseUnit() const;
	virtual MathStructure &convertToBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertFromBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertToBaseUnit(MathStructure &mvalue) const;
	virtual MathStructure &convertFromBaseUnit(MathStructure &mvalue) const;
	virtual MathStructure convertToBaseUnit() const;
	virtual MathStructure convertFromBaseUnit() const;
	virtual int baseExponent(int exp = 1) const;
	virtual int type() const;
	/** Returns the subtype of the unit, corresponding to which subsubclass the object belongs to.
	*
	* @returns ::UnitSubtype.
	*/
	virtual int subtype() const;
	/** If specified unit is a base unit for this unit, directly or with other units in between.
	* Equivalent to u->isParentOf(this).
	*/
	virtual bool isChildOf(Unit *u) const;
	/** If this unit is a base unit for specified unit, directly or with other units in between.
	* Equivalent to u->isChildOf(this).
	*/
	virtual bool isParentOf(Unit *u) const;
	virtual bool hasComplexRelationTo(Unit *u) const;
	/** Converts a value from specified unit and exponent to this unit.
	* value * (unit^exponent) = new value * (this^new exponent)
	* This function cannot convert to or from CompositeUnit.
	*
	* @param u Unit to convert from.
	* @param[in,out] mvalue Quantity value.
	* @param[in,out] exp Exponent.
	* @returns true if the value was successfully converted.
	*/
	bool convert(Unit *u, MathStructure &mvalue, MathStructure &exp) const;
	/** Converts a value from specified unit and exponent to this unit.
	* value * unit = new value * this
	* This function cannot convert to or from CompositeUnit.
	*
	* @param u Unit to convert from.
	* @param[in,out] mvalue Quantity value.
	* @returns true if the value was successfully converted.
	*/
	bool convert(Unit *u, MathStructure &mvalue) const;
	MathStructure convert(Unit *u, bool *converted = NULL) const;

};


/// An unit with relation to another unit
/**
* Alias units is defined in relation to another unit.
* For example, hour are defined as an alias unit that equals 60 minutes which in turn is defined in relation to seconds.
*
* Alias units have an associated base unit, exponent and relation expression.
* For more complex relations an inverse relation can also be specified for conversion back from the base unit.
* The base unit must not necessarily be of the base unit class and it is recommended that an alias unit is defined in relation to the closest unit
* (ex. 1ft = 3 hands, 1 hand = 4 in, and 1 in = 0.0254 m).
*
* The relation is usually just a number that tells how large quantity of the base unit is needed to get the alias unit (alias unit = base unit * relation).
* More complex units can specify the relation as a full-blown expression where '\x' is replaced by the quantity of the base unit and
* '\y' is the exponent.
* For example, Degrees Celsius has the relation "\x + 273.15" and the inverse relation "\x - 273.15" to the base unit Kelvin.
* For simple relations, the reversion is automatic and ought not be defined separately.
*
* The precision property inherited from ExpressionItem defines the precision of the relation.
*
* The exponent defines the exponential relation to the base unit, so that the alias unit equals the base unit raised to the exponent.
* For simple unit relations this gives: alias unit = relation * base unit^exponent.
*
* Alias units normally have three different names defined for use in expressions - abbreviation (ex. "m"), singular ("meter") and plural ("meters").
*/
class AliasUnit : public Unit {

  protected:

	string svalue, sinverse, suncertainty;
	int i_exp, i_mix, i_mix_min;
	Unit *o_unit;

  public:

	AliasUnit(string cat_, string name_, string plural_, string singular_, string title_, Unit *alias, string relation = "1", int exp = 1, string inverse = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	AliasUnit(const AliasUnit *unit);		
	AliasUnit();			
	virtual ~AliasUnit();

	virtual ExpressionItem *copy() const;
	virtual void set(const ExpressionItem *item);
	
	virtual Unit* baseUnit() const;
	virtual Unit* firstBaseUnit() const;
	virtual void setBaseUnit(Unit *alias);
	virtual string expression() const;
	virtual string inverseExpression() const;
	virtual string uncertainty() const;
	/**
	* Sets the relation expression.
	*/
	virtual void setExpression(string relation);
	/**
	* Sets the inverse relation expression.
	*/
	virtual void setInverseExpression(string inverse);
	virtual void setUncertainty(string standard_uncertainty);
	virtual MathStructure &convertToFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertFromFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertToBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertFromBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertToBaseUnit(MathStructure &mvalue) const;
	virtual MathStructure &convertFromBaseUnit(MathStructure &mvalue) const;
	virtual MathStructure convertToBaseUnit() const;
	virtual MathStructure convertFromBaseUnit() const;
	virtual int baseExponent(int exp = 1) const;
	virtual void setExponent(int exp);
	virtual int firstBaseExponent() const;
	virtual int mixWithBase() const;
	virtual int mixWithBaseMinimum() const;
	virtual void setMixWithBase(int combine_priority = 1);
	virtual void setMixWithBaseMinimum(int combine_minimum);
	virtual int subtype() const;
	virtual bool isChildOf(Unit *u) const;
	virtual bool isParentOf(Unit *u) const;
	virtual bool hasComplexExpression() const;
	virtual bool hasComplexRelationTo(Unit *u) const;

};

/// A subunit in a CompositeUnit
/**
* Should normally not be used directly.
*/
class AliasUnit_Composite : public AliasUnit {

  protected:

	Prefix *prefixv;

  public:

	AliasUnit_Composite(Unit *alias, int exp = 1, Prefix *prefix_ = NULL);
	AliasUnit_Composite(const AliasUnit_Composite *unit);			
	virtual ~AliasUnit_Composite();

	virtual ExpressionItem *copy() const;
	virtual void set(const ExpressionItem *item);
	
	virtual string print(bool plural_, bool short_, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual Prefix *prefix() const;
	virtual int prefixExponent() const;	
	virtual void set(Unit *u, int exp = 1, Prefix *prefix_ = NULL);
	virtual MathStructure &convertToFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;
	virtual MathStructure &convertFromFirstBaseUnit(MathStructure &mvalue, MathStructure &mexp) const;

};

/// A unit consisting of a number of other units
/**
* Composite units are defined by a unit expression with multiple units.
* Composite units often have an alias unit associated with them, as they do not have a reference name on their own.
* For example, a joule is defined as an alias defined in relation to a composite unit defined as "Newton * meter".
*
* The names of composite units is only used to reference the unit in definitions of other units.
* They can not be used in expressions.
*
* Composite units is definited as a composition of units.
* The units, with prefixes and exponents, can either be added one by one with add() or
* parsed from an expression (ex. "cm^3/g) with setBaseExpression().
*/
class CompositeUnit : public Unit {
	
	protected:
	
		string sshort;
		vector<AliasUnit_Composite*> units;

	public:

		CompositeUnit(string cat_, string name_, string title_ = "", string base_expression_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
		CompositeUnit(const CompositeUnit *unit);
		virtual ~CompositeUnit();
		virtual ExpressionItem *copy() const;
		virtual void set(const ExpressionItem *item);
		/** Adds a sub/base unit with specified exponent and an optional prefix.
		*
		* @param u Unit.
		* @param exp Exponent.
		* @param prefix Prefix.
		*/
		virtual void add(Unit *u, int exp = 1, Prefix *prefix = NULL);
		/** Retrieves information about a sub/base unit 
		*
		* @param index Index starting at 1.
		* @param[out] exp Exponent.
		* @param[out] prefix Prefix.
		* @returns Sub/base unit (AliasUnit_Composite::firstBaseUnit()).
		*/
		virtual Unit *get(size_t index, int *exp = NULL, Prefix **prefix = NULL) const;
		virtual void setExponent(size_t index, int exp);
		virtual void setPrefix(size_t index, Prefix *prefix);
		/** Returns the number of sub/base units */
		virtual size_t countUnits() const;
		virtual size_t find(Unit *u) const;
		virtual void del(size_t index);
		/** Prints out the sub/base units with prefixes and exponents.
		* This is the representation of the unit in expressions.
		*/
		virtual string print(bool plural_, bool short_, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
		virtual int subtype() const;
		/** If this unit contains a sub/base unit with a relation to the specified unit.
		 */
		virtual bool containsRelativeTo(Unit *u) const;
		/** Creates a MathStructure with the sub/base units of the unit.
		*/
		virtual MathStructure generateMathStructure(bool make_division = false, bool set_null_prefixes = false) const;
		virtual void setBaseExpression(string base_expression_);
		/** Removes all sub/base units. */
		virtual void clear();
};


#endif
