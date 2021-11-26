/*
    Qalculate (library)

    Copyright (C) 2003-2006, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PREFIX_H
#define PREFIX_H


#include <libqalculate/includes.h>
#include <libqalculate/Number.h>
#include <libqalculate/ExpressionItem.h>

/** @file */

///Types for prefix classes.
typedef enum {
	PREFIX_DECIMAL,
	PREFIX_BINARY,
	PREFIX_NUMBER
} PrefixType;

///Abstract class for prefixes.
/** A prefix is prepended to a unit to specify a quantity multiplicator. A prefix has a numerical value which raised to the units power defines the quantity. In for example the expression "3 kilometers", meter is the unit, 3 is regular quantity, and kilo is a prefix with a value 1000, thus the example equals "3000 meters". If the unit instead had been squared, the value of the prefix would have been raised by two and the total quantity would have been 3.000.000.

Prefixes can have up to free different three names -- a long name, a short name and a short unicode name. The unicode name is an alternative to the short name that is preferred if unicode characters can be displayed. The names or used to reference the prefix in mathematical expressions and to display a prefix in a result.
 */
class Prefix {
  protected:
	std::vector<ExpressionName> names;
  public:
	/** Create a prefix.
	*
	* @param long_name Long name.
	* @param short_name Short name.
	* @param unicode_name Unicode name.
 	*/
  	Prefix(std::string long_name, std::string short_name = "", std::string unicode_name = "");
	virtual ~Prefix();
	/** Returns the short name of the prefix.
	*
	* @param return_long_if_no_short If the long name shall be returned if the prefix has not got a short name (if it is empty).
	* @param use_unicode If a unicode version of the name is allowed and preferred.
	* @returns The short name of the prefix.
 	*/
	const std::string &shortName(bool return_long_if_no_short = true, bool use_unicode = false) const;
	/** Returns the long name of the prefix.
	*
	* @param return_short_if_no_long If the short name shall be returned if the prefix has not got a long name (if it is empty).
	* @param use_unicode If a unicode version of the name is allowed and preferred.
	* @returns The long name of the prefix.
 	*/
	const std::string &longName(bool return_short_if_no_long = true, bool use_unicode = false) const;
	/** Returns the unicode name of the prefix.
	*
	* @param return_short_if_no_uni If the short name shall be returned if the prefix has not got a unicode name (if it is empty).
	* @returns The unicode name of the prefix.
 	*/
	const std::string &unicodeName(bool return_short_if_no_uni = true) const;
	/** Sets the short name of the prefix.
	*
	* @param short_name The new short name for the prefix.
 	*/
	void setShortName(std::string short_name);
	/** Sets the long name of the prefix.
	*
	* @param long_name The new long name for the prefix.
 	*/
	void setLongName(std::string long_name);
	/** Sets the unicode name of the prefix. The unicode name is an alternative to the short name that is preferred if unicode characters can be displayed.
	*
	* @param unicode_name The new unicode name for the prefix.
 	*/
	void setUnicodeName(std::string unicode_name);
	/** Returns a preferred name of the prefix.
	*
	* @param short_default If a short name is preferred.
	* @param use_unicode If a unicode name is preferred.
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns A preferred name.
 	*/
	const std::string &name(bool short_default = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	const std::string &referenceName() const;
	/** Returns the name that best fulfils provided criterias. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param plural If a name in plural form is preferred.
	* @param reference If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	const ExpressionName &preferredName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criterias and is suitable for user input. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param plural If a name in plural form is preferred.
	* @param reference If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	const ExpressionName &preferredInputName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criterias and is suitable for display. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	const ExpressionName &preferredDisplayName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns name for an index (starting at one). All functions can be traversed by starting at index one and increasing the index until empty_expression_name is returned.
	*
	* @param index Index of name.
	* @returns Name for index or empty_expression_name if not found.
	*/
	const ExpressionName &getName(size_t index) const;
	/** Changes a name. If a name for the provided index is not present, it is added (equivalent to addName(ename, index)).
	*
	* @param ename The new name.
	* @param index Index of name to change.
	*/
	void setName(const ExpressionName &ename, size_t index = 1);
	/** Changes the text string of a name. If a name for the provided index is not present, it is added (equivalent to addName(sname, index)).
	*
	* @param sname The new name text string.
	* @param index Index of name to change.
	*/
	void setName(std::string sname, size_t index);
	void addName(const ExpressionName &ename, size_t index = 0);
	void addName(std::string sname, size_t index = 0);
	size_t countNames() const;
	/** Removes all names. */
	void clearNames();
	/** Removes all names that are not used for reference (ExpressionName.reference = true). */
	void clearNonReferenceNames();
	void removeName(size_t index);
	/** Checks if the prefix has a name with a specific text string.
	*
	* @param sname A text string to look for (not case sensitive)
	* @param case_sensitive If the name is case sensitive.
	* @returns Index of the name with the given text string or zero if such a name was not found.
	*/
	size_t hasName(const std::string &sname, bool case_sensitive = true) const;
	/** Checks if the prefix has a name with a specific case sensitive text string.
	*
	* @param sname A text string to look for (case sensitive)
	* @returns Index of the name with the given text string or zero if such a name was not found.
	*/
	size_t hasNameCaseSensitive(const std::string &sname) const;
	/** Searches for a name with specific properties.
	*
	* @param abbreviation If the name must be abbreviated. 1=true, 0=false, -1=ignore.
	* @param use_unicode If the name must have unicode characters. 1=true, 0=false, -1=ignore.
	* @param plural If the name must be in plural form. 1=true, 0=false, -1=ignore.
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The first found name with the specified properties or empty_expression_name if none found.
	*/
	const ExpressionName &findName(int abbreviation = -1, int use_unicode = -1, int plural = -1, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the value of the prefix.
	*
	* @param nexp The power of the prefixed unit.
	* @returns The value of the prefix.
 	*/
	virtual Number value(const Number &nexp) const = 0;
	/** Returns the value of the prefix.
	*
	* @param iexp The power of the prefixed unit.
	* @returns The value of the prefix.
 	*/
	virtual Number value(int iexp) const = 0;
	/** Returns the value of the prefix.
	*
	* @returns The value of the prefix.
 	*/
	virtual Number value() const = 0;
	/** Returns type, subclass, of the prefix. This can be PREFIX_DECIMAL for prefixes of the class DecimalPrefix, PREFIX_BINARY for BinaryPrefix, or PREFIX_NUMBER for NumberPrefix.
	*
	* @returns The type of the prefix.
 	*/
	virtual int type() const = 0;

};

///A decimal (metric) prefix.
/** A metric or decimal prefix has an integer exponent which with a base of ten constitutes the value of the prefix (value=10^exponent).
 */
class DecimalPrefix : public Prefix {
  protected:
	int exp;
  public:
	/** Create a decimal prefix.
	*
	* @param exp10 Exponent for the value.
	* @param long_name Long name.
	* @param short_name Short name.
	* @param unicode_name Unicode name.
 	*/
  	DecimalPrefix(int exp10, std::string long_name, std::string short_name = "", std::string unicode_name = "");
	~DecimalPrefix();
	/** Returns the exponent.
	*
	* @param iexp Exponent of the unit.
	* @returns The exponent of the prefix.
 	*/
	int exponent(int iexp = 1) const;
	/** Returns the exponent.
	*
	* @param nexp Exponent of the unit.
	* @returns The exponent of the prefix.
 	*/
	Number exponent(const Number &nexp) const;
	/** Sets the exponent of the prefix.
	*
	* @param iexp New exponent for the prefix.
 	*/
	void setExponent(int iexp);
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

///A binary prefix.
/** A Binary prefix has an integer exponent which with a base of two constitutes the value of the prefix (value=2^exponent).
 */
class BinaryPrefix : public Prefix {
  protected:
	int exp;
  public:
	/** Create a binary prefix.
	*
	* @param exp2 Exponent for the value.
	* @param long_name Long name.
	* @param short_name Short name.
	* @param unicode_name Unicode name.
 	*/
  	BinaryPrefix(int exp2, std::string long_name, std::string short_name = "", std::string unicode_name = "");
	~BinaryPrefix();
	/** Returns the exponent.
	*
	* @param iexp Exponent of the unit.
	* @returns The exponent of the prefix.
 	*/
	int exponent(int iexp = 1) const;
	/** Returns the exponent.
	*
	* @param nexp Exponent of the unit.
	* @returns The exponent of the prefix.
 	*/
	Number exponent(const Number &nexp) const;
	/** Sets the exponent of the prefix.
	*
	* @param iexp New exponent for the prefix.
 	*/
	void setExponent(int iexp);
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

///A prefix with a free numerical value.
/** A prefix without any predefined base, which can use any number.
 */
class NumberPrefix : public Prefix {
  protected:
	Number o_number;
  public:
	/** Create a number prefix.
	*
	* @param nr Value of the prefix.
	* @param long_name Long name.
	* @param short_name Short name.
	* @param unicode_name Unicode name.
 	*/
  	NumberPrefix(const Number &nr, std::string long_name, std::string short_name = "", std::string unicode_name = "");
	~NumberPrefix();
	/** Sets the value of the prefix.
	*
	* @param nr New value for the prefix.
 	*/
	void setValue(const Number &nr);
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

#endif
