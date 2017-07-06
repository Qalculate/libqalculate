/*
    Qalculate (library)

    Copyright (C) 2004-2006, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DATA_SET_H
#define DATA_SET_H

#include <libqalculate/includes.h>
#include <libqalculate/Function.h>

/** @file */

typedef vector<DataProperty*>::iterator DataObjectPropertyIter;

/// A a data set object.
/** Data objects consist of property-value pairs. */
class DataObject {

  protected:

	vector<DataProperty*> properties;
	vector<string> s_properties;
	vector<string> s_nonlocalized_properties;
	vector<MathStructure*> m_properties;
	vector<int> a_properties;
	DataSet *parent;
	bool b_uchanged;
	
  public:
 
	/** Create a data object.
	*
	* @param parent_set Data set that the object will belong to.
	*/
	DataObject(DataSet *parent_set);

	/** Unset (erase value) a property.
	*
	* @param property Property to unset.
	*/
	void eraseProperty(DataProperty *property);
	/** Set value for a property.
	*
	* @param property Property to set (must belong to parent data set).
	* @param s_vale Value for the property.
	* @param is_approximate If the value is approximate. 1 for approximate, 0 for exact, -1 for property default.
	*/
	void setProperty(DataProperty *property, string s_value, int is_approximate = -1);
	/** Set an untranslated value for a key property. Used when a text value is translated, but the original value still is needed as a reference key.
	*
	* @param property Property to set (must belong to parent data set).
	* @param s_vale Value for the property.
	*/
	void setNonlocalizedKeyProperty(DataProperty *property, string s_value);
	
	/** Returns parsed value for a property. Parses the text string value if not parsed before 
	*
	* @param property Property to read.
	* @returns Parsed value or NULL if property value is not set.
	*/
	const MathStructure *getPropertyStruct(DataProperty *property);
	/** Returns unparsed value for a property.
	*
	* @param property Property to read.
	* @param[out] is_approximate If the value is approximate. Is set to 1 for approximate, 0 for exact, -1 for property default, if not NULL.
	* @returns Unparsed value or empty string if property value is not set.
	*/
	const string &getProperty(DataProperty *property, int *is_approximate = NULL);
	/** Returns unparsed untranslated value for a key property. Used when a text value is translated, but the original value still is needed as a reference key.
	*
	* @param property Property to read.
	* @returns Unparsed untranslated value or empty string if property value is not set.
	*/
	const string &getNonlocalizedKeyProperty(DataProperty *property);
	/** Returns value for a property in a format suitable for use in expressions with unit appended.
	*
	* @param property Property to read.
	* @returns Value in input format or empty string if property value is not set.
	*/
	string getPropertyInputString(DataProperty *property);
	/** Returns value for a property in a format suitable for display with unit appended.
	*
	* @param property Property to read.
	* @returns Value in display format or empty string if property value is not set.
	*/
	string getPropertyDisplayString(DataProperty *property);

	/** If the object has been modified by the end user (if setUserModified() has been called).
	*
	* @returns true if the object has been modified by the user.
	*/
	bool isUserModified() const;
	/** Specify if the object has been modified by the end user.
	*
	* @param user_modified true if the object has been modified by the user.
	*/
	void setUserModified(bool user_modified = true);

	/** Returns the data set that the object belongs to.
	*
	* @returns Parent data set.
	*/
	DataSet *parentSet() const;

};

typedef enum {
	PROPERTY_EXPRESSION,
	PROPERTY_NUMBER,
	PROPERTY_STRING
} PropertyType;

/// A data set property.
/** Property definitions for use with data set objects. */
class DataProperty {

  protected:

	vector<string> names;
	vector<bool> name_is_ref;
	string sdescr, stitle, sunit;
	MathStructure *m_unit;
	bool b_approximate, b_brackets, b_key, b_case, b_hide;
	DataSet *parent;
	PropertyType ptype;
	bool b_uchanged;
	
  public:

	/** Create a data property.
	*
	* @param s_name Property name (initial) used for reference.
	* @param s_title Descriptive name/title.
	* @param s_description Description.
	*/
	DataProperty(DataSet *parent_set, string s_name = "", string s_title = "", string s_description = "");
	DataProperty(const DataProperty &dp);
	
	void set(const DataProperty &dp);
	void setName(string s_name, bool is_ref = false);
	void setNameIsReference(size_t index = 1, bool is_ref = true);
	bool nameIsReference(size_t index = 1) const;
	void clearNames();
	void addName(string s_name, bool is_ref = false, size_t index = 0);
	size_t hasName(const string &s_name);
	size_t countNames() const;
	const string &getName(size_t index = 1) const;
	const string &getReferenceName() const;
	void setTitle(string s_title);
	const string &title(bool return_name_if_no_title = true) const;
	void setDescription(string s_description);
	const string &description() const;
	void setUnit(string s_unit);
	const string &getUnitString() const;
	const MathStructure *getUnitStruct();
	string getInputString(const string &valuestr);
	string getDisplayString(const string &valuestr);
	MathStructure *generateStruct(const string &valuestr, int is_approximate = -1);
	void setKey(bool is_key = true);
	bool isKey() const;
	void setHidden(bool is_hidden = true);
	bool isHidden() const;
	void setCaseSensitive(bool is_case_sensitive = true);
	bool isCaseSensitive() const;
	void setUsesBrackets(bool uses_brackets = true);
	bool usesBrackets() const;
	void setApproximate(bool is_approximate = true);
	bool isApproximate() const;
	void setPropertyType(PropertyType property_type);
	PropertyType propertyType() const;
	
	bool isUserModified() const;
	void setUserModified(bool user_modified = true);
	
	DataSet *parentSet() const;
	
};

typedef vector<DataProperty*>::iterator DataPropertyIter;
typedef vector<DataObject*>::iterator DataObjectIter;

/// A data set.
/** This is a simple database class for storage of many grouped values, when ordinary variables is not practical.
*
* A data set consists of properties and objects, with values for the properties. Qalculate! includes for example a "Planets" data set with properties such as name, mass, speed and density, and an object for each planet in solar system.
*
* A data set is also mathemtical function, dataset(object, property), which retrieves values for objects and properties.
* Data sets can be saved and loaded from a XML file.
*/
class DataSet : public MathFunction {

  protected:
  
	string sfile, scopyright;
	bool b_loaded;
	vector<DataProperty*> properties;
	vector<DataObject*> objects;
	
  public:
  
  	DataSet(string s_category = "", string s_name = "", string s_default_file = "", string s_title = "", string s_description = "", bool is_local = true);
	DataSet(const DataSet *o);
	
	ExpressionItem *copy() const;
	void set(const ExpressionItem *item);
	int subtype() const;
	
	void setCopyright(string s_copyright);
	const string &copyright() const;
	void setDefaultDataFile(string s_file);
	const string &defaultDataFile() const;
	
	void setDefaultProperty(string property);
	const string &defaultProperty() const;
	
	virtual int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);
	
	bool loadObjects(const char *file_name = NULL, bool is_user_defs = true);
	int saveObjects(const char *file_name = NULL, bool save_global = false);
	bool objectsLoaded() const;
	void setObjectsLoaded(bool objects_loaded);
	
	void addProperty(DataProperty *dp);
	void delProperty(DataProperty *dp);
	void delProperty(DataPropertyIter *it);
	DataProperty *getPrimaryKeyProperty();
	DataProperty *getProperty(string property);
	DataProperty *getFirstProperty(DataPropertyIter *it);
	DataProperty *getNextProperty(DataPropertyIter *it);
	const string &getFirstPropertyName(DataPropertyIter *it);
	const string &getNextPropertyName(DataPropertyIter *it);
	
	void addObject(DataObject *o);
	void delObject(DataObject *o);
	void delObject(DataObjectIter *it);
	DataObject *getObject(string object);
	DataObject *getObject(const MathStructure &object);
	DataObject *getFirstObject(DataObjectIter *it);
	DataObject *getNextObject(DataObjectIter *it);
	
	const MathStructure *getObjectProperyStruct(string property, string object);
	const string &getObjectProperty(string property, string object);
	string getObjectPropertyInputString(string property, string object);
	string getObjectPropertyDisplayString(string property, string object);
	
	string printProperties(string object);
	string printProperties(DataObject *o);
		
};

/// Data property function argument.
class DataPropertyArgument : public Argument {

  protected:
  
  	DataSet *o_data;
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	DataPropertyArgument(DataSet *data_set, string name_ = "", bool does_test = true, bool does_error = true);
	DataPropertyArgument(const DataPropertyArgument *arg);
	~DataPropertyArgument();
	int type() const;
	Argument *copy() const;
	string print() const;
	DataSet *dataSet() const;
	void setDataSet(DataSet *data_set);
	
};

/// Data object function argument.
class DataObjectArgument : public Argument {

  protected:
  
  	DataSet *o_data;
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	DataObjectArgument(DataSet *data_set, string name_ = "", bool does_test = true, bool does_error = true);
	DataObjectArgument(const DataObjectArgument *arg);
	~DataObjectArgument();
	int type() const;
	Argument *copy() const;
	string print() const;
	DataSet *dataSet() const;
	void setDataSet(DataSet *data_set);
	
};

#endif
