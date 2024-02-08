/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSIONITEM_H
#define EXPRESSIONITEM_H

#include <libqalculate/includes.h>

/** @file */

/// A name for an expression item (function, variable or unit)
/** An expression name has a text string representing a name and boolean values describing the names properties.
*/
struct ExpressionName {

	/// If the name is an abbreviation.
	bool abbreviation;
	/// If the name has a suffix. If set to true, the part of the name after an underscore should be treated as a suffix.
	bool suffix;
	/// If the name contains unicode characters.
	bool unicode;
	/// If the name is in plural form.
	bool plural;
	/// If the name shall be used as a fixed reference. If this is set to true, the name will kept as it is in addition to translations of it.
	bool reference;
	/// If the name is unsuitable for user input.
	bool avoid_input;
	/// If the name is case sensitive. The default behavior is that abbreviations are case sensitive and other names are not.
	bool case_sensitive;
	/// Use only for completion (useful for unicode letter alternatives)
	bool completion_only;
	/// The name.
	std::string name;

	class ExpressionItem_p *priv;

	/** Create an empty expression name. All properties are set to false.
	*/
	ExpressionName();
	/** Create an expression name. All properties are set to false, unless the name only has one character in which case abbreviation and case_sensitive is set to true.
	*
	* @param sname The name.
	*/
	ExpressionName(std::string sname);

	void operator = (const ExpressionName &ename);
	bool operator == (const ExpressionName &ename) const;
	bool operator != (const ExpressionName &ename) const;

	int underscoreRemovalAllowed() const;
	std::string formattedName(int type, bool capitalize, bool html_suffix = false, int unicode_suffix = 0, bool remove_typename = false, bool hide_underscore = false, bool *was_formatted = NULL, bool *was_capitalized = NULL) const;

};

/// Abstract base class for functions, variables and units.
/**
* Expression items have one or more names used to reference it in mathematical expressions and display them in a result.
* Each name must be fully unique, with the exception that functions can have names used by other types of items
* (for example "min" is used as a name for the minute unit but also for a function returning smallest value in a vector).
*
* Items have an optional title and description for information to the end user.
* The category property is used to organize items, so that the end user can easily find them.
* Subcategories are separated by a slash, '/' (ex. "Physical Constants/Electromagnetic Constants").
*
* A local item is created/edited by the end user.
*
* A builtin item has defining properties that can/should not be edited by the user and is usually an item not loaded from the definition files.
*
* An inactive item can not be used in expressions and can share the name of an active item.
*
* The hidden property defines if the item should be hidden from the end user.
*
* Before an item can be used in expressions, it must be added to the Calculator object using CALCULATOR->addExpressionItem().
* It is then said to be registered.
*
* To delete an ExpressionItem object you should use destroy() to make sure that the item is removed from the Calculator and does not have any referrer.
*
*/
class ExpressionItem {

  protected:

	std::string scat, stitle, sdescr;
	bool b_local, b_changed, b_builtin, b_approx, b_active, b_registered, b_hidden, b_destroyed;
	int i_ref, i_precision;
	std::vector<ExpressionItem*> v_refs;
	std::vector<ExpressionName> names;

  public:

	ExpressionItem(std::string cat_, std::string name_, std::string title_ = "", std::string descr_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	ExpressionItem();
	virtual ~ExpressionItem();

	virtual ExpressionItem *copy() const = 0;
	virtual void set(const ExpressionItem *item);

	virtual bool destroy();

	bool isRegistered() const;
	/// For internal use.
	void setRegistered(bool is_registered);

	virtual const std::string &name(bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual const std::string &referenceName() const;

	/** Returns the name that best fulfils provided criteria. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param plural If a name in plural form is preferred.
	* @param reference If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criteria and is suitable for user input. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param plural If a name in plural form is preferred.
	* @param reference If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredInputName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criteria and is suitable for display. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param plural If a name in plural form is preferred.
	* @param reference If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredDisplayName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns name for an index (starting at one). All functions can be traversed by starting at index one and increasing the index until empty_expression_name is returned.
	*
	* @param index Index of name.
	* @returns Name for index or empty_expression_name if not found.
	*/
	virtual const ExpressionName &getName(size_t index) const;
	/** Changes a name. If a name for the provided index is not present, it is added (equivalent to addName(ename, index, force)).
	*
	* @param ename The new name.
	* @param index Index of name to change.
	* @param force If true, expression items with conflicting names are replaced, otherwise . Only applies if the item is registered.
	*/
	virtual void setName(const ExpressionName &ename, size_t index = 1, bool force = true);
	/** Changes the text string of a name. If a name for the provided index is not present, it is added (equivalent to addName(sname, index, force)).
	*
	* @param sname The new name text string.
	* @param index Index of name to change.
	* @param force If true, expression items with conflicting names are replaced, otherwise . Only applies if the item is registered.
	*/
	virtual void setName(std::string sname, size_t index, bool force = true);
	virtual void addName(const ExpressionName &ename, size_t index = 0, bool force = true);
	virtual void addName(std::string sname, size_t index = 0, bool force = true);
	virtual size_t countNames() const;
	/** Removes all names. */
	virtual void clearNames();
	/** Removes all names that are not used for reference (ExpressionName.reference = true). */
	virtual void clearNonReferenceNames();
	virtual void removeName(size_t index);
	/** Checks if the expression item has a name with a specific text string.
	*
	* @param sname A text string to look for (not case sensitive)
	* @param case_sensitive If the name is case sensitive.
	* @returns Index of the name with the given text string or zero if such a name was not found.
	*/
	virtual size_t hasName(const std::string &sname, bool case_sensitive = true) const;
	/** Checks if the expression item has a name with a specific case sensitive text string.
	*
	* @param sname A text string to look for (case sensitive)
	* @returns Index of the name with the given text string or zero if such a name was not found.
	*/
	virtual size_t hasNameCaseSensitive(const std::string &sname) const;
	/** Searches for a name with specific properties.
	*
	* @param abbreviation If the name must be abbreviated. 1=true, 0=false, -1=ignore.
	* @param use_unicode If the name must have unicode characters. 1=true, 0=false, -1=ignore.
	* @param plural If the name must be in plural form. 1=true, 0=false, -1=ignore.
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The first found name with the specified properties or empty_expression_name if none found.
	*/
	virtual const ExpressionName &findName(int abbreviation = -1, int use_unicode = -1, int plural = -1, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;

	/** Returns the title, descriptive name, of the item.
	*
	* @param return_name_if_no_title If true, a name is returned if the title string is empty (using preferredName(false, use_unicode, false, false, can_display_unicode_string_function, can_display_unicode_string_arg)).
	* @param use_unicode If a name with unicode characters can be displayed/is preferred (passed to preferredName()).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected (passed to preferredName()).
	* @param can_display_unicode_string_arg Argument to pass to the above test function (passed to preferredName()).
	* @returns Item title.
	*/
	virtual const std::string &title(bool return_name_if_no_title = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;

	/** Sets the title, descriptive name, of the item. The title can not be used in expressions.
	*
	* @param title_ The new title.
	*/
	virtual void setTitle(std::string title_);

	/** Returns the expression items description.
	*
	* @returns Description.
	*/
	virtual const std::string &description() const;
	/** Sets the expression items description.
	*
	* @param descr_ Description.
	*/
	virtual void setDescription(std::string descr_);

	/** Returns the category that the expression item belongs to. Subcategories are separated by '/'.
	*
	* @returns Category.
	*/
	virtual const std::string &category() const;
	/** Sets which category the expression belongs to. Subcategories are separated by '/'.
	*
	* @param cat_ Category.
	*/
	virtual void setCategory(std::string cat_);

	/** If the object has been changed since it was created/loaded. */
	virtual bool hasChanged() const;
	virtual void setChanged(bool has_changed);

	virtual bool isLocal() const;
	virtual bool setLocal(bool is_local = true, int will_be_active = -1);

	virtual bool isBuiltin() const;

	/** If the item is approximate or exact.
	* Note that an actual value associated with the item might have a have a lower precision.
	* For, for example, a mathematical function this defines the precision of the formula, not the result.
	*
	* @returns true if the item is approximate
	*/
	virtual bool isApproximate() const;
	virtual void setApproximate(bool is_approx = true);

	/** Returns precision of the item, if it is approximate.
	* Note that an actual value associated with the item might have a have a lower precision.
	* For, for example, a mathematical function this defines the precision of the formula, not the result.
	*/
	virtual int precision() const;
	virtual void setPrecision(int prec);

	/** Returns if the expression item is active and can be used in expressions.
	*
	* @returns true if active.
	*/
	virtual bool isActive() const;
	virtual void setActive(bool is_active);

	virtual bool isHidden() const;
	virtual void setHidden(bool is_hidden);

	/** The reference count is not used to delete the expression item when it becomes zero, but to stop from being deleted while it is in use.
	*/
	virtual int refcount() const;
	virtual void ref();
	virtual void unref();
	virtual void ref(ExpressionItem *o);
	virtual void unref(ExpressionItem *o);
	virtual ExpressionItem *getReferencer(size_t index = 1) const;
	virtual bool changeReference(ExpressionItem *o_from, ExpressionItem *o_to);

	/** Returns the type of the expression item, corresponding to which subclass the object belongs to.
	*
	* @returns ::ExpressionItemType.
	*/
	virtual int type() const = 0;
	/** Returns the subtype of the expression item, corresponding to which subsubclass the object belongs to.
	*
	* @returns Subtype/subsubclass.
	*/
	virtual int subtype() const = 0;

	virtual int id() const;

};

#endif
