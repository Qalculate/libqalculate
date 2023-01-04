/*
    Qalculate

    Copyright (C) 2008  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Function.h"
#include "util.h"
#include "Calculator.h"
#include "MathStructure.h"
#include "Variable.h"
#include "Number.h"
#include "Unit.h"
#include "BuiltinFunctions.h"

#include <limits.h>

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
	using std::unordered_map;
#elif 	defined(__GNUC__)
#	ifndef __has_include
#	define __has_include(x) 0
#	endif

#	if (defined(__clang__) && __has_include(<tr1/unordered_map>)) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#		include <tr1/unordered_map>
		namespace Sgi = std;
#		define unordered_map std::tr1::unordered_map
#	else
#		if __GNUC__ < 3
#			include <hash_map.h>
			namespace Sgi { using ::hash_map; }; // inherit globals
#		else
#			include <ext/hash_map>
#			if __GNUC__ == 3 && __GNUC_MINOR__ == 0
				namespace Sgi = std;               // GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
#			endif
#		endif
#		define unordered_map Sgi::hash_map
#	endif
#else      // ...  there are other compilers, right?
	namespace Sgi = std;
#	define unordered_map Sgi::hash_map
#endif

#include "MathStructure-support.h"

using std::string;
using std::vector;
using std::iterator;
using std::cout;
using std::endl;

class MathFunction_p {
	public:
		unordered_map<size_t, Argument*> argdefs;
		std::vector<std::string> v_subs_calc;
};

MathFunction::MathFunction(string name_, int argc_, int max_argc_, string cat_, string title_, string descr_, bool is_active) : ExpressionItem(cat_, name_, title_, descr_, false, true, is_active) {
	priv = new MathFunction_p;
	// arc = min number of arguments, max_argc = max arguments (if max_argc=-1, place additional arguments in vector)
	argc = argc_;
	if(max_argc_ < 0 || argc < 0) {
		if(argc < 0) argc = 0;
		max_argc = -1;
	} else if(max_argc_ < argc) {
		max_argc = argc;
	} else {
		max_argc = max_argc_;
		// arguments not required, must have a default value (by default 0)
		for(int i = 0; i < max_argc - argc; i++) {
			default_values.push_back("0");
		}
	}
	last_argdef_index = 0;
}
MathFunction::MathFunction(const MathFunction *function) {
	priv = new MathFunction_p;
	set(function);
}
MathFunction::MathFunction() {
	priv = new MathFunction_p;
	argc = 0;
	max_argc = 0;
	last_argdef_index = 0;
}
MathFunction::~MathFunction() {
	clearArgumentDefinitions();
	delete priv;
}

void MathFunction::set(const ExpressionItem *item) {
	if(item->type() == TYPE_FUNCTION) {
		MathFunction *f = (MathFunction*) item;
		argc = f->minargs();
		max_argc = f->maxargs();
		default_values.clear();
		for(int i = argc + 1; i <= max_argc || !f->getDefaultValue(i).empty(); i++) {
			setDefaultValue(i, f->getDefaultValue(i));
		}
		last_argdef_index = f->lastArgumentDefinitionIndex();
		scondition = f->condition();
		clearArgumentDefinitions();
		for(size_t i = 1; i <= f->lastArgumentDefinitionIndex(); i++) {
			if(f->getArgumentDefinition(i)) {
				setArgumentDefinition(i, f->getArgumentDefinition(i)->copy());
			}
		}
	}
	ExpressionItem::set(item);
}
int MathFunction::type() const {
	return TYPE_FUNCTION;
}
int MathFunction::subtype() const {
	return SUBTYPE_FUNCTION;
}
int MathFunction::id() const {
	return 0;
}

string MathFunction::example(bool raw_format, string name_string) const {
	// example text
	if(raw_format) return sexample;
	string str = sexample;
	// $name is replaced function name (or provided string)
	gsub("$name", name_string.empty() ? name() : name_string, str);
	// adjust decimal and group separators
	return CALCULATOR->localizeExpression(str);
}
void MathFunction::setExample(string new_example) {
	sexample = new_example;
}

/*int MathFunction::countArgOccurrence(size_t arg_) {
	if((int) arg_ > argc && max_argc < 0) {
		arg_ = argc + 1;
	}
	if(argoccs.find(arg_) != argoccs.end()) {
		return argoccs[arg_];
	}
	return 1;
}*/
int MathFunction::args() const {
	return max_argc;
}
int MathFunction::minargs() const {
	return argc;
}
int MathFunction::maxargs() const {
	return max_argc;
}
string MathFunction::condition() const {
	return scondition;
}
void MathFunction::setCondition(string expression) {
	scondition = expression;
	remove_blank_ends(scondition);
}
bool MathFunction::testCondition(const MathStructure &vargs) {
	// test condition for arguments
	if(scondition.empty()) {
		return true;
	}
	// create a temporary function from the condition expression (for handling av arguments)
	CALCULATOR->beginTemporaryStopMessages();
	UserFunction test_function("", "CONDITION_TEST_FUNCTION", scondition, false, argc, "", "", max_argc < 0 && !default_values.empty() && scondition.find("\\v") == string::npos && scondition.find("\\w") == string::npos ? argc + default_values.size() : max_argc);
	MathStructure vargs2(vargs);
	if(test_function.maxargs() > 0 && vargs2.size() > (size_t) test_function.maxargs()) vargs2.resizeVector(test_function.maxargs(), m_zero);
	MathStructure mstruct(test_function.MathFunction::calculate(vargs2));
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	mstruct.eval(eo);
	CALCULATOR->endTemporaryStopMessages();
	// check if result is true
	if(!mstruct.isNumber() || !mstruct.number().getBoolean()) {
		if(CALCULATOR->showArgumentErrors() && !CALCULATOR->aborted()) {
			CALCULATOR->error(true, _("%s() requires that %s"), name().c_str(), printCondition().c_str(), NULL);
		}
		return false;
	}
	return true;
}
string MathFunction::printCondition() {
	if(scondition.empty()) return scondition;
	// replace arguments (represented by \x, \y, \z, \a, etc.) with argument definition names
	string str = scondition;
	string svar, argstr;
	Argument *arg;
	int i_args = maxargs();
	bool b_v = i_args < 0 && (default_values.empty() || scondition.find("\\v") != string::npos || scondition.find("\\w") != string::npos);
	if(b_v) i_args = minargs() + 2;
	else if(i_args < 0) i_args = minargs() + default_values.size();
	for(int i = 0; i < i_args; i++) {
		svar = '\\';
		if(b_v && maxargs() < 0 && i >= minargs()) {
			svar += (char) ('v' + i - minargs());
		} else {
			if('x' + i > 'z') {
				svar += (char) ('a' + i - 3);
			} else {
				svar += 'x' + i;
			}
		}
		size_t i2 = 0;
		while(true) {
			if((i2 = str.find(svar, i2)) != string::npos) {
				if(maxargs() < 0 && i > minargs()) {
					arg = getArgumentDefinition(i + 1);
					if(!arg) arg = getArgumentDefinition(i);
				} else {
					arg = getArgumentDefinition(i + 1);
				}
				argstr = "\"";
				if(!arg || arg->name().empty()) {
					argstr += _("argument");
					argstr += " ";
					if(maxargs() < 0 && i > minargs()) {
						argstr += i2s(i);
					} else {
						argstr += i2s(i + 1);
					}
				} else {
					argstr += arg->name();
				}
				argstr += "\"";
				str.replace(i2, 2, argstr);
			} else {
				break;
			}
		}
	}
	return str;
}
int MathFunction::args(const string &argstr, MathStructure &vargs, const ParseOptions &parseoptions) {
	// read arguments from expression (e.g. "52, 2" from expression "sin(52, 2)"); used in Calculator::parse()
	ParseOptions po = parseoptions;
	MathStructure *unended_function = po.unended_function;
	po.unended_function = NULL;
	vargs.clearVector();
	int start_pos = 0;
	bool in_cit1 = false, in_cit2 = false;
	int pars = 0;
	int itmp = 0;
	string str = argstr, stmp;
	remove_blank_ends(str);
	Argument *arg;
	bool last_is_vctr = false, vctr_started = false;
	if(maxargs() > 0) {
		arg = getArgumentDefinition(maxargs());
		// if last argument is vector or maximum number of arguments is 1 and the only argument allows vector
		last_is_vctr = arg && ((arg->type() == ARGUMENT_TYPE_VECTOR) || (maxargs() == 1 && arg->handlesVector()));
	}
	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		switch(str[str_index]) {
			// argument does not end within parentheses, brackets or quotes
			case LEFT_VECTOR_WRAP_CH: {}
			case LEFT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2) {
					pars++;
				}
				break;
			}
			case RIGHT_VECTOR_WRAP_CH: {}
			case RIGHT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2 && pars > 0) {
					pars--;
				}
				break;
			}
			case '\"': {
				if(in_cit1) {
					in_cit1 = false;
				} else if(!in_cit2) {
					in_cit1 = true;
				}
				break;
			}
			case '\'': {
				if(in_cit2) {
					in_cit2 = false;
				} else if(!in_cit1) {
					in_cit1 = true;
				}
				break;
			}
			// argument separator
			case COMMA_CH: {
				if(pars == 0 && !in_cit1 && !in_cit2) {
					itmp++;
					// read one argument
					if(itmp <= maxargs() || args() < 0) {
						// index is <= max number of arguments
						stmp = str.substr(start_pos, str_index - start_pos);
						remove_blank_ends(stmp);
						arg = getArgumentDefinition(itmp);
						// index is greater than minimum number of arguments,
						// and maximum number of arguments is unlimited, use the last argument definition
						if(!arg && itmp > argc && args() < 0 && itmp > (int) last_argdef_index && (int) last_argdef_index > argc) {
							 arg = priv->argdefs[last_argdef_index];
						}
						if(stmp.empty()) {
							if(arg) {
								// if index has argument definition, use for parsing
								MathStructure *mstruct = new MathStructure();
								arg->parse(mstruct, getDefaultValue(itmp), po);
								vargs.addChild_nocopy(mstruct);
							} else {
								MathStructure *mstruct = new MathStructure();
								CALCULATOR->parse(mstruct, getDefaultValue(itmp));
								vargs.addChild_nocopy(mstruct);
							}
						} else {
							if(arg) {
								MathStructure *mstruct = new MathStructure();
								arg->parse(mstruct, stmp, po);
								vargs.addChild_nocopy(mstruct);
							} else {
								MathStructure *mstruct = new MathStructure();
								CALCULATOR->parse(mstruct, stmp, po);
								vargs.addChild_nocopy(mstruct);
							}
						}
					} else if(last_is_vctr) {
						// if last argument is a vector, use additional arguments to fill the vector
						if(!vctr_started) {
							if(!vargs[vargs.size() - 1].isVector() || vargs[vargs.size() - 1].size() != 1) {
								vargs[vargs.size() - 1].transform(STRUCT_VECTOR);
							}
							vctr_started = true;
						}
						stmp = str.substr(start_pos, str_index - start_pos);
						remove_blank_ends(stmp);
						if(stmp.empty()) {
							MathStructure *mstruct = new MathStructure();
							getArgumentDefinition(maxargs())->parse(mstruct, getDefaultValue(itmp));
							vargs[vargs.size() - 1].addChild_nocopy(mstruct);
						} else {
							MathStructure *mstruct = new MathStructure();
							getArgumentDefinition(maxargs())->parse(mstruct, stmp, po);
							vargs[vargs.size() - 1].addChild_nocopy(mstruct);
						}
						vargs.childUpdated(vargs.size());
					} else {
						CALCULATOR->error(false, _n("Additional arguments for function %s() were ignored. Function can only use %s argument.", "Additional arguments for function %s() were ignored. Function can only use %s arguments.", maxargs()), name().c_str(), i2s(maxargs()).c_str(), NULL);
					}
					start_pos = str_index + 1;
				}
				break;
			}
		}
	}
	// generate unended function information
	// used from Calculator::parse() for display of continuous parsing info
	// handling is incomplete if used separately
	if(!str.empty()) {
		itmp++;
		po.unended_function = unended_function;
		if(itmp <= maxargs() || args() < 0) {
			stmp = str.substr(start_pos, str.length() - start_pos);
			remove_blank_ends(stmp);
			arg = getArgumentDefinition(itmp);
			if(!arg && itmp > argc && args() < 0 && itmp > (int) last_argdef_index && (int) last_argdef_index > argc) {
				 arg = priv->argdefs[last_argdef_index];
			}
			if(stmp.empty()) {
				if(arg) {
					MathStructure *mstruct = new MathStructure();
					arg->parse(mstruct, getDefaultValue(itmp));
					vargs.addChild_nocopy(mstruct);
				} else {
					MathStructure *mstruct = new MathStructure();
					CALCULATOR->parse(mstruct, getDefaultValue(itmp));
					vargs.addChild_nocopy(mstruct);
				}
			} else {
				if(arg) {
					MathStructure *mstruct = new MathStructure();
					arg->parse(mstruct, stmp, po);
					vargs.addChild_nocopy(mstruct);
				} else {
					MathStructure *mstruct = new MathStructure();
					CALCULATOR->parse(mstruct, stmp, po);
					vargs.addChild_nocopy(mstruct);
				}
			}
		} else if(last_is_vctr) {
			if(!vctr_started) {
				if(!vargs[vargs.size() - 1].isVector() || vargs[vargs.size() - 1].size() != 1) {
					vargs[vargs.size() - 1].transform(STRUCT_VECTOR);
				}
				vctr_started = true;
			}
			stmp = str.substr(start_pos, str.length() - start_pos);
			remove_blank_ends(stmp);
			if(stmp.empty()) {
				MathStructure *mstruct = new MathStructure();
				getArgumentDefinition(maxargs())->parse(mstruct, getDefaultValue(itmp));
				vargs[vargs.size() - 1].addChild_nocopy(mstruct);
			} else {
				MathStructure *mstruct = new MathStructure();
				getArgumentDefinition(maxargs())->parse(mstruct, stmp, po);
				vargs[vargs.size() - 1].addChild_nocopy(mstruct);
			}
			vargs.childUpdated(vargs.size());
		} else {
			CALCULATOR->error(false, _n("Additional arguments for function %s() were ignored. Function can only use %s argument.", "Additional arguments for function %s() were ignored. Function can only use %s arguments.", maxargs()), name().c_str(), i2s(maxargs()).c_str(), NULL);
		}
	}
	if(unended_function && !unended_function->isFunction()) {
		unended_function->set(vargs);
		unended_function->setType(STRUCT_FUNCTION);
		unended_function->setFunction(this);
		while((int) unended_function->size() < itmp) {
			unended_function->addChild(m_undefined);
		}
	}
	// append default values
	if((itmp < maxargs() && itmp >= minargs()) || (maxargs() < 0 && itmp >= minargs() && (size_t) itmp - minargs() < default_values.size() && !default_values[itmp - minargs()].empty())) {
		int itmp2 = itmp;
		if(id() == FUNCTION_ID_LOGN) CALCULATOR->error(false, _("log() with a single argument is considered ambiguous. Please use ln() or log10() instead."), NULL);
		while((size_t) itmp2 - minargs() < default_values.size() && (maxargs() > 0 || !default_values[itmp2 - minargs()].empty())) {
			arg = getArgumentDefinition(itmp2 + 1);
			MathStructure *mstruct = new MathStructure();
			if(arg) arg->parse(mstruct, default_values[itmp2 - minargs()]);
			else CALCULATOR->parse(mstruct, default_values[itmp2 - minargs()]);
			vargs.addChild_nocopy(mstruct);
			itmp2++;
		}
	}
	return itmp;
}
size_t MathFunction::lastArgumentDefinitionIndex() const {
	return last_argdef_index;
}
Argument *MathFunction::getArgumentDefinition(size_t index) {
	if(priv->argdefs.find(index) != priv->argdefs.end()) {
		return priv->argdefs[index];
	}
	return NULL;
}
void MathFunction::clearArgumentDefinitions() {
	for(unordered_map<size_t, Argument*>::iterator it = priv->argdefs.begin(); it != priv->argdefs.end(); ++it) {
		delete it->second;
	}
	priv->argdefs.clear();
	last_argdef_index = 0;
	setChanged(true);
}
void MathFunction::setArgumentDefinition(size_t index, Argument *argdef) {
	if(priv->argdefs.find(index) != priv->argdefs.end()) {
		delete priv->argdefs[index];
	}
	if(argdef) {
		priv->argdefs[index] = argdef;
		if(index > last_argdef_index) {
			last_argdef_index = index;
		}
		argdef->setIsLastArgument((int) index == maxargs());
	} else {
		priv->argdefs.erase(index);
		if(index == last_argdef_index) {
			last_argdef_index = 0;
			if(!priv->argdefs.empty()) {
				for(size_t i = index - 1; i > 0; i--) {
					if(priv->argdefs.find(i) != priv->argdefs.end()) {
						last_argdef_index = i;
						break;
					}
				}
			}
		}
	}
	setChanged(true);
}
bool MathFunction::testArgumentCount(int itmp) {
	if(itmp >= minargs()) {
		if(itmp > maxargs() && maxargs() >= 0 && (maxargs() > 1 || !getArgumentDefinition(1) || !getArgumentDefinition(1)->handlesVector())) {
			CALCULATOR->error(false, _n("Additional arguments for function %s() were ignored. Function can only use %s argument.", "Additional arguments for function %s() were ignored. Function can only use %s arguments.", maxargs()), name().c_str(), i2s(maxargs()).c_str(), NULL);
		}
		return true;
	}
	string str;
	Argument *arg;
	bool b = false;
	for(int i = 1; i <= minargs(); i++) {
		arg = getArgumentDefinition(i);
		if(i > 1) {
			str += CALCULATOR->getComma();
			str += " ";
		}
		if(arg && !arg->name().empty()) {
			str += arg->name();
			b = true;
		} else {
			str += "?";
		}
	}
	if(b) {
		CALCULATOR->error(true, _n("You need at least %s argument (%s) in function %s().", "You need at least %s arguments (%s) in function %s().", minargs()), i2s(minargs()).c_str(), str.c_str(), name().c_str(), NULL);
	} else {
		CALCULATOR->error(true, _n("You need at least %s argument in function %s().", "You need at least %s arguments in function %s().", minargs()), i2s(minargs()).c_str(), name().c_str(), NULL);
	}
	return false;
}
MathStructure MathFunction::createFunctionMathStructureFromVArgs(const MathStructure &vargs) {
	MathStructure mstruct(this, NULL);
	for(size_t i = 0; i < vargs.size(); i++) {
		mstruct.addChild(vargs[i]);
	}
	return mstruct;
}
MathStructure MathFunction::createFunctionMathStructureFromSVArgs(vector<string> &svargs) {
	MathStructure mstruct(this, NULL);
	for(size_t i = 0; i < svargs.size(); i++) {
		mstruct.addChild(svargs[i]);
	}
	return mstruct;
}
MathStructure MathFunction::calculate(const string &argv, const EvaluationOptions &eo) {
/*	MathStructure vargs;
	args(argv, vargs, eo.parse_options);
	return calculate(vargs, eo);*/
	MathStructure fmstruct(parse(argv, eo.parse_options));
	fmstruct.calculateFunctions(eo);
	return fmstruct;
}
MathStructure MathFunction::parse(const string &argv, const ParseOptions &po) {
	MathStructure vargs;
	args(argv, vargs, po);
	vargs.setType(STRUCT_FUNCTION);
	vargs.setFunction(this);
	return vargs;
	//return createFunctionMathStructureFromVArgs(vargs);
}
int MathFunction::parse(MathStructure &mstruct, const string &argv, const ParseOptions &po) {
	int n = args(argv, mstruct, po);
	mstruct.setType(STRUCT_FUNCTION);
	mstruct.setFunction(this);
	return n;
}
bool MathFunction::testArguments(MathStructure &vargs) {
	size_t last = 0;
	for(unordered_map<size_t, Argument*>::iterator it = priv->argdefs.begin(); it != priv->argdefs.end(); ++it) {
		if(it->first > last) {
			last = it->first;
		}
		if(it->second && it->first > 0 && it->first <= vargs.size()) {
			// for symbols arguments with zero or undefined value, search the first argument for a symbol
			if(it->second->type() == ARGUMENT_TYPE_SYMBOLIC && (vargs[it->first - 1].isZero() || vargs[it->first - 1].isUndefined())) {
				vargs[it->first - 1] = vargs[0].find_x_var();
				if(vargs[it->first - 1].isUndefined() && vargs[0].isVariable() && vargs[0].variable()->isKnown()) vargs[it->first - 1] = ((KnownVariable*) vargs[0].variable())->get().find_x_var();
				if(vargs[it->first - 1].isUndefined()) {
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure mtest(vargs[0]);
					mtest.eval();
					vargs[it->first - 1] = mtest.find_x_var();
					CALCULATOR->endTemporaryStopMessages();
				}
				if(vargs[it->first - 1].isUndefined()) {
					vargs[it->first - 1].set(CALCULATOR->getVariableById(VARIABLE_ID_X), true);
					CALCULATOR->error(false, _("No unknown variable/symbol was found."), NULL);
				}
			}
			if(!it->second->test(vargs[it->first - 1], it->first, this)) return false;
		}
	}
	if(max_argc < 0 && (int) last > argc && priv->argdefs.find(last) != priv->argdefs.end()) {
		for(size_t i = last + 1; i <= vargs.size(); i++) {
			if(!priv->argdefs[last]->test(vargs[i - 1], i, this)) {
				return false;
			}
		}
	}
	return testCondition(vargs);
}
MathStructure MathFunction::calculate(MathStructure &vargs, const EvaluationOptions &eo) {
	int itmp = vargs.size();
	if(testArgumentCount(itmp)) {
		appendDefaultValues(vargs);
		MathStructure mstruct;
		int ret = 0;
		if(!testArguments(vargs) || (ret = calculate(mstruct, vargs, eo)) < 1) {
			if(ret < 0) {
				ret = -ret;
				if(maxargs() > 0 && ret > maxargs()) {
					if(mstruct.isVector()) {
						for(size_t arg_i = 0; arg_i < vargs.size() && arg_i < mstruct.size(); arg_i++) {
							vargs.setChild(mstruct[arg_i], arg_i + 1);
						}
					}
				} else if(ret <= (int) vargs.size()) {
					vargs.setChild(mstruct, ret);
				}
			}
			return createFunctionMathStructureFromVArgs(vargs);
		}
		if(precision() >= 0 && (precision() < mstruct.precision() || mstruct.precision() < 1)) mstruct.setPrecision(precision(), true);
		if(isApproximate()) mstruct.setApproximate(true, true);
		return mstruct;
	} else {
		return createFunctionMathStructureFromVArgs(vargs);
	}
}
int MathFunction::calculate(MathStructure&, const MathStructure&, const EvaluationOptions&) {
	//mstruct = createFunctionMathStructureFromVArgs(vargs);
	return 0;
}
void MathFunction::setDefaultValue(size_t arg_, string value_) {
	if((int) arg_ > argc) {
		while(default_values.size() < arg_ - argc) default_values.push_back("");
		default_values[arg_ - argc - 1] = value_;
	}
}
const string &MathFunction::getDefaultValue(size_t arg_) const {
	if((int) arg_ > argc && (int) default_values.size() >= (int) arg_ - argc) {
		return default_values[arg_ - argc - 1];
	}
	return empty_string;
}
void MathFunction::appendDefaultValues(MathStructure &vargs) {
	if((int) vargs.size() < minargs()) return;
	while((int) vargs.size() < maxargs() || (maxargs() < 0 && (size_t) vargs.size() - minargs() < default_values.size() && !default_values[vargs.size() - minargs()].empty())) {
		Argument *arg = getArgumentDefinition(vargs.size() + 1);
		if(arg) {
			MathStructure *mstruct = new MathStructure();
			arg->parse(mstruct, default_values[vargs.size() - minargs()]);
			vargs.addChild_nocopy(mstruct);
		} else {
			MathStructure *mstruct = new MathStructure();
			CALCULATOR->parse(mstruct, default_values[vargs.size() - minargs()]);
			vargs.addChild_nocopy(mstruct);
		}
	}
}
int MathFunction::stringArgs(const string &argstr, vector<string> &svargs) {
	svargs.clear();
	int start_pos = 0;
	bool in_cit1 = false, in_cit2 = false;
	int pars = 0;
	int itmp = 0;
	string str = argstr, stmp;
	remove_blank_ends(str);
	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		switch(str[str_index]) {
			case LEFT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2) {
					pars++;
				}
				break;
			}
			case RIGHT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2 && pars > 0) {
					pars--;
				}
				break;
			}
			case '\"': {
				if(in_cit1) {
					in_cit1 = false;
				} else if(!in_cit2) {
					in_cit1 = true;
				}
				break;
			}
			case '\'': {
				if(in_cit2) {
					in_cit2 = false;
				} else if(!in_cit1) {
					in_cit1 = true;
				}
				break;
			}
			case COMMA_CH: {
				if(pars == 0 && !in_cit1 && !in_cit2) {
					itmp++;
					if(itmp <= maxargs() || args() < 0) {
						stmp = str.substr(start_pos, str_index - start_pos);
						remove_blank_ends(stmp);
						remove_parenthesis(stmp);
						remove_blank_ends(stmp);
						if(stmp.empty()) {
							stmp = getDefaultValue(itmp);
						}
						svargs.push_back(stmp);
					}
					start_pos = str_index + 1;
				}
				break;
			}
		}
	}
	if(!str.empty()) {
		itmp++;
		if(itmp <= maxargs() || args() < 0) {
			stmp = str.substr(start_pos, str.length() - start_pos);
			remove_blank_ends(stmp);
			remove_parenthesis(stmp);
			remove_blank_ends(stmp);
			if(stmp.empty()) {
				stmp = getDefaultValue(itmp);
			}
			svargs.push_back(stmp);
		}
	}
	if((itmp < maxargs() && itmp >= minargs()) || (maxargs() < 0 && itmp >= minargs() && (size_t) itmp - minargs() < default_values.size() && !default_values[itmp - minargs()].empty())) {
		int itmp2 = itmp;
		while((size_t) itmp2 - minargs() < default_values.size() && (maxargs() > 0 || !default_values[itmp2 - minargs()].empty())) {
			svargs.push_back(default_values[itmp2 - minargs()]);
			itmp2++;
		}
	}
	return itmp;
}

MathStructure MathFunction::produceVector(const MathStructure &vargs, int begin, int end) {
	if(begin < 1) {
		begin = minargs() + 1;
		if(begin < 1) begin = 1;
	}
	if(end < 1 || end >= (int) vargs.size()) {
		end = vargs.size();
	}
	if(begin == 1 && vargs.size() == 1) {
		if(vargs[0].isVector()) {
			return vargs[0];
		} else {
			return vargs;
		}
	}
	MathStructure mstruct;
	vargs.getRange(begin, end, mstruct);
	MathStructure mstruct2;
	return mstruct.flattenVector(mstruct2);
}
MathStructure MathFunction::produceArgumentsVector(const MathStructure &vargs, int begin, int end) {
	if(begin < 1) {
		begin = minargs() + 1;
		if(begin < 1) begin = 1;
	}
	if(end < 1 || end >= (int) vargs.size()) {
		end = vargs.size();
	}
	if(begin == 1 && vargs.size() == 1) {
		return vargs;
	}
	MathStructure mstruct;
	return vargs.getRange(begin, end, mstruct);
}
bool MathFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool MathFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool MathFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return representsPositive(vargs, allow_units);}
bool MathFunction::representsNonPositive(const MathStructure &vargs, bool allow_units) const {return representsNegative(vargs, allow_units);}
bool MathFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return representsBoolean(vargs) || representsEven(vargs, allow_units) || representsOdd(vargs, allow_units);}
bool MathFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return representsReal(vargs, allow_units) || representsComplex(vargs, allow_units);}
bool MathFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return representsInteger(vargs, allow_units);}
bool MathFunction::representsNonComplex(const MathStructure &vargs, bool allow_units) const {return representsReal(vargs, allow_units);}
bool MathFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return representsRational(vargs, allow_units);}
bool MathFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool MathFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return representsPositive(vargs, allow_units) || representsNegative(vargs, allow_units);}
bool MathFunction::representsEven(const MathStructure&, bool) const {return false;}
bool MathFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool MathFunction::representsUndefined(const MathStructure&) const {return false;}
bool MathFunction::representsBoolean(const MathStructure&) const {return false;}
bool MathFunction::representsNonMatrix(const MathStructure &vargs) const {
	return representsNumber(vargs, true);
}
bool MathFunction::representsScalar(const MathStructure &vargs) const {
	return representsNonMatrix(vargs);
}

UserFunction::UserFunction(string cat_, string name_, string formula_, bool is_local, int argc_, string title_, string descr_, int max_argc_, bool is_active) : MathFunction(name_, argc_, max_argc_, cat_, title_, descr_, is_active) {
	b_local = is_local;
	b_builtin = false;
	setFormula(formula_, argc_, max_argc_);
	setChanged(false);
}
UserFunction::UserFunction(const UserFunction *function) {
	set(function);
}
string UserFunction::formula() const {
	return sformula;
}
string UserFunction::internalFormula() const {
	return sformula_calc;
}
ExpressionItem *UserFunction::copy() const {
	return new UserFunction(this);
}
void UserFunction::set(const ExpressionItem *item) {
	if(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_USER_FUNCTION) {
		sformula = ((UserFunction*) item)->formula();
		v_subs.clear();
		v_precalculate.clear();
		for(size_t i = 1; i <= ((UserFunction*) item)->countSubfunctions(); i++) {
			v_subs.push_back(((UserFunction*) item)->getSubfunction(i));
			v_precalculate.push_back(((UserFunction*) item)->subfunctionPrecalculated(i));
		}
		if(!v_subs.empty()) {
			bool b = hasChanged();
			setFormula(sformula);
			setChanged(b);
		}
		sformula_calc = ((UserFunction*) item)->internalFormula();
	}
	MathFunction::set(item);
}
int UserFunction::subtype() const {
	return SUBTYPE_USER_FUNCTION;
}
extern string format_and_print(const MathStructure &mstruct);
bool replace_intervals_f(MathStructure &mstruct) {
	// replace intervals with temporary variables
	if(mstruct.isNumber() && (mstruct.number().isInterval(false) || (CALCULATOR->usesIntervalArithmetic() && mstruct.number().precision() >= 0))) {
		Variable *v = new KnownVariable("", format_and_print(mstruct), mstruct);
		v->setTitle("\b");
		mstruct.set(v, true);
		v->destroy();
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_intervals_f(mstruct[i])) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
bool replace_f_interval(MathStructure &mstruct, const EvaluationOptions &eo) {
	// replace interval() and uncertainty() with numbers with intervals, if possible
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_INTERVAL && mstruct.size() == 2) {
		if(mstruct[0].isNumber() && mstruct[1].isNumber()) {
			Number nr;
			if(nr.setInterval(mstruct[0].number(), mstruct[1].number())) {
				mstruct.set(nr, true);
				return true;
			}
		} else {
			MathStructure m1(mstruct[0]);
			MathStructure m2(mstruct[1]);
			if(create_interval(mstruct, m1, m2)) return true;
			m1.eval(eo);
			m2.eval(eo);
			if(create_interval(mstruct, m1, m2)) return true;
		}
		return false;
	} else if(eo.interval_calculation != INTERVAL_CALCULATION_NONE && mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY && mstruct.size() == 3) {
		if(mstruct[0].isNumber() && mstruct[1].isNumber()) {
			Number nr(mstruct[0].number());
			if(mstruct[2].number().getBoolean()) {
				nr.setRelativeUncertainty(mstruct[1].number(), false);
			} else {
				nr.setUncertainty(mstruct[1].number(), false);
			}
			mstruct.set(nr, true);
			return true;
		} else {
			MathStructure m1(mstruct[0]);
			MathStructure m2(mstruct[1]);
			if(mstruct[2].number().getBoolean()) {
				m1.eval(eo);
				m2.eval(eo);
				if(m1.isNumber() && m2.isNumber()) {
					Number nr(m1.number());
					nr.setRelativeUncertainty(m2.number(), false);
					mstruct.set(nr, true);
					return true;
				}
				m1 = mstruct[0];
				m2 = mstruct[1];
				mstruct.setToChild(1, true);
				mstruct *= m_one;
				mstruct.last() -= m2;
				mstruct.transformById(FUNCTION_ID_INTERVAL);
				m1 *= m_one;
				m1.last() += m2;
				mstruct.addChild(m1);
				return 1;
			} else {
				if(set_uncertainty(m1, m2, eo, false)) return true;
				m1.eval(eo);
				m2.eval(eo);
				if(set_uncertainty(m1, m2, eo, true)) return true;
				m1 = mstruct[0];
				m2 = mstruct[1];
				mstruct.setToChild(1);
				mstruct -= m2;
				mstruct.transformById(FUNCTION_ID_INTERVAL);
				m1 += m2;
				mstruct.addChild(m1);
			}
			replace_f_interval(mstruct, eo);
			return true;

		}
		return false;
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_f_interval(mstruct[i], eo)) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
int UserFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po;
	if(b_local) po.angle_unit = eo.parse_options.angle_unit;
	if(args() != 0) {
		string stmp = sformula_calc;
		string svar;
		string v_str, w_str;
		vector<string> v_strs;
		vector<size_t> v_id;
		size_t i2 = 0;
		int i_args = maxargs();
		if(i_args < 0) {
			i_args = minargs();
		}
		if(priv->v_subs_calc.size() != v_subs.size()) {
			size_t i = priv->v_subs_calc.size();
			if(priv->v_subs_calc.size() > v_subs.size()) {i = 0; priv->v_subs_calc.clear();}
			for(; i < v_subs.size(); i++) {
				priv->v_subs_calc.push_back(v_subs[i]);
			}
		}
		for(int i = 0; i < i_args; i++) {
			MathStructure *mv = new MathStructure(vargs[i]);
			Argument *arg = getArgumentDefinition(i + 1);
			if((!arg || !arg->tests() || arg->type() == ARGUMENT_TYPE_FREE) && mv->containsInterval(true, false, false, 0, true)) {
				size_t count = 0;
				for(size_t i3 = 0; i3 < 1 || (maxargs() < 0 && i3 < 3); i3++) {
					svar = '\\';
					if(i3 == 1) {
						svar += 'v';
					} else if(i3 == 2) {
						svar += 'w';
					} else if('x' + i > 'z') {
						svar += (char) ('a' + i - 3);
					} else {
						svar += 'x' + i;
					}
					size_t pos = 0;
					while((pos = sformula_calc.find(svar, pos)) != string::npos) {
						pos += svar.length();
						count++;
					}
					for(size_t i2 = 0; i2 < priv->v_subs_calc.size(); i2++) {
						pos = 0;
						size_t c2 = 0;
						while((pos = priv->v_subs_calc[i2].find(svar, pos)) != string::npos) {
							pos += svar.length();
							c2++;
						}
						string svar2 = "\\";
						svar2 += i2s(i2 + 1);
						pos = 0;
						size_t c_sub = 0;
						while((pos = sformula_calc.find(svar2, pos)) != string::npos) {
							pos += svar2.length();
							c_sub++;
						}
						count += c2 * c_sub;
					}
				}
				if(count > 1) {
					replace_f_interval(*mv, eo);
					replace_intervals_f(*mv);
				}
			}
			v_id.push_back(CALCULATOR->addId(mv, true));
			v_strs.push_back(LEFT_PARENTHESIS ID_WRAP_LEFT);
			v_strs[i] += i2s(v_id[i]);
			v_strs[i] += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
		}
		if(maxargs() < 0) {
			if(stmp.find("\\v") != string::npos) {
				v_id.push_back(CALCULATOR->addId(new MathStructure(produceVector(vargs)), true));
				v_str = LEFT_PARENTHESIS ID_WRAP_LEFT;
				v_str += i2s(v_id[v_id.size() - 1]);
				v_str += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
			}
			if(stmp.find("\\w") != string::npos) {
				v_id.push_back(CALCULATOR->addId(new MathStructure(produceArgumentsVector(vargs)), true));
				w_str = LEFT_PARENTHESIS ID_WRAP_LEFT;
				w_str += i2s(v_id[v_id.size() - 1]);
				w_str += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
			}
		}

		for(size_t i = 0; i < priv->v_subs_calc.size(); i++) {
			if(subfunctionPrecalculated(i + 1)) {
				string str = priv->v_subs_calc[i];
				for(int i3 = 0; i3 < i_args; i3++) {
					svar = '\\';
					if('x' + i3 > 'z') {
						svar += (char) ('a' + i3 - 3);
					} else {
						svar += 'x' + i3;
					}
					i2 = 0;
					while(true) {
						if((i2 = str.find(svar, i2)) != string::npos) {
							if(i2 != 0 && str[i2 - 1] == '\\') {
								i2 += 2;
							} else {
								str.replace(i2, 2, v_strs[i3]);
							}
						} else {
							break;
						}
					}
				}
				size_t v_id_i = v_id.size() - 1;
				string str2;
				for(int i3 = i - 1; i3 >= 0; i3--) {
					i2 = 0;
					svar = '\\';
					svar += i2s(i3 + 1);
					while(true) {
						if((i2 = str.find(svar, i2)) != string::npos) {
							if(i2 != 0 && str[i2 - 1] == '\\') {
								i2 += 2;
							} else {
								if(subfunctionPrecalculated(i3 + 1)) {
									if(str2.empty()) {
										str2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
										str2 += i2s(v_id[v_id_i]);
										str2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
									}
									str.replace(i2, svar.size(), str2);
								} else {
									str.replace(i2, svar.size(), string("(") + priv->v_subs_calc[i] + ")");
								}
							}
						} else {
							break;
						}
					}
					if(subfunctionPrecalculated(i3 + 1)) {
						v_id_i--;
						str2 = "";
					}
				}
				if(maxargs() < 0) {
					i2 = 0;
					while(true) {
						if((i2 = str.find("\\v")) != string::npos) {
							if(i2 != 0 && str[i2 - 1] == '\\') {
								i2 += 2;
							} else {
								str.replace(i2, 2, v_str);
							}
						} else {
							break;
						}
					}
					i2 = 0;
					while(true) {
						if((i2 = str.find("\\w")) != string::npos) {
							if(i2 != 0 && str[i2 - 1] == '\\') {
								i2 += 2;
							} else {
								str.replace(i2, 2, w_str);
							}
						} else {
							break;
						}
					}
				}
				MathStructure *v_mstruct = new MathStructure();
				CALCULATOR->parse(v_mstruct, str, po);
				v_mstruct->eval(eo);
				v_id.push_back(CALCULATOR->addId(v_mstruct, true));
				str = LEFT_PARENTHESIS ID_WRAP_LEFT;
				str += i2s(v_id[v_id.size() - 1]);
				str += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				i2 = 0;
				svar = '\\';
				svar += i2s(i + 1);
				while(true) {
					if((i2 = stmp.find(svar, i2)) != string::npos) {
						if(i2 != 0 && stmp[i2 - 1] == '\\') {
							i2 += 2;
						} else {
							stmp.replace(i2, svar.size(), str);
						}
					} else {
						break;
					}
				}
			} else {
				size_t v_id_i = v_id.size() - 1;
				string str = priv->v_subs_calc[i];
				string str2;
				for(int i3 = i - 1; i3 >= 0; i3--) {
					i2 = 0;
					svar = '\\';
					svar += i2s(i3 + 1);
					while(true) {
						if((i2 = str.find(svar, i2)) != string::npos) {
							if(i2 != 0 && str[i2 - 1] == '\\') {
								i2 += 2;
							} else {
								if(subfunctionPrecalculated(i3 + 1)) {
									if(str2.empty()) {
										str2 = LEFT_PARENTHESIS ID_WRAP_LEFT;
										str2 += i2s(v_id[v_id_i]);
										str2 += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
									}
									str.replace(i2, svar.size(), str2);
								} else {
									str.replace(i2, svar.size(), string("(") + priv->v_subs_calc[i] + ")");
								}
							}
						} else {
							break;
						}
					}
					if(subfunctionPrecalculated(i3 + 1)) {
						v_id_i--;
						str2 = "";
					}
				}
				i2 = 0;
				svar = '\\';
				svar += i2s(i + 1);
				while(true) {
					if((i2 = stmp.find(svar, i2)) != string::npos) {
						if(i2 != 0 && stmp[i2 - 1] == '\\') {
							i2 += svar.size();
						} else {
							stmp.replace(i2, svar.size(), string("(") + str + ")");
						}
					} else {
						break;
					}
				}
			}
		}
		for(int i = 0; i < i_args; i++) {
			svar = '\\';
			if('x' + i > 'z') {
				svar += (char) ('a' + i - 3);
			} else {
				svar += 'x' + i;
			}
			i2 = 0;
			while(true) {
				if((i2 = stmp.find(svar, i2)) != string::npos) {
					if(i2 != 0 && stmp[i2 - 1] == '\\') {
						i2 += 2;
					} else {
						stmp.replace(i2, 2, v_strs[i]);
					}
				} else {
					break;
				}
			}
		}
		if(maxargs() < 0) {
			i2 = 0;
			while(true) {
				if((i2 = stmp.find("\\v")) != string::npos) {
					if(i2 != 0 && stmp[i2 - 1] == '\\') {
						i2 += 2;
					} else {
						stmp.replace(i2, 2, v_str);
					}
				} else {
					break;
				}
			}
			i2 = 0;
			while(true) {
				if((i2 = stmp.find("\\w")) != string::npos) {
					if(i2 != 0 && stmp[i2 - 1] == '\\') {
						i2 += 2;
					} else {
						stmp.replace(i2, 2, w_str);
					}
				} else {
					break;
				}
			}
		}
		while(true) {
			if((i2 = stmp.find("\\\\")) != string::npos) {
				stmp.replace(i2, 2, "\\");
			} else {
				break;
			}
		}
		CALCULATOR->parse(&mstruct, stmp, po);
		for(size_t i = 0; i < v_id.size(); i++) {
			CALCULATOR->delId(v_id[i]);
		}
		if(precision() >= 0) mstruct.setPrecision(precision(), true);
		if(isApproximate()) mstruct.setApproximate(true, true);
	} else {
		CALCULATOR->parse(&mstruct, sformula_calc, po);
		if(precision() >= 0) mstruct.setPrecision(precision(), true);
		if(isApproximate()) mstruct.setApproximate(true, true);
	}
	return 1;
}
void UserFunction::setFormula(string new_formula, int argc_, int max_argc_) {
	setChanged(true);
	sformula = new_formula;
	default_values.clear();
	priv->v_subs_calc.clear();
	if(sformula.empty() && v_subs.empty()) {
		sformula_calc = new_formula;
		argc = 0;
		max_argc = 0;
		return;
	}
	for(size_t sub_i = 0; sub_i < v_subs.size(); sub_i++) {
		priv->v_subs_calc.push_back(v_subs[sub_i]);
	}
	if(argc_ < 0) {
		argc_ = 0, max_argc_ = 0;
		string svar, svar_o, svar_v;
		bool optionals = false, b;
		size_t i3 = 0, i4 = 0, i5 = 0;
		size_t i2 = 0;
		for(int i = 0; i < 26; i++) {
			begin_loop_in_set_formula:
			i4 = 0; i5 = 0;
			svar = '\\';
			svar_o = '\\';
			if('x' + i > 'z') svar += (char) ('a' + i - 3);
			else svar += 'x' + i;
			if('X' + i > 'Z') svar_o += (char) ('A' + i - 3);
			else svar_o += 'X' + i;
			before_find_in_set_formula:
			if(i < 24 && (i2 = new_formula.find(svar_o, i4)) != string::npos) {
				if(i2 > 0 && new_formula[i2 - 1] == '\\') {
					i4 = i2 + 2;
					goto before_find_in_set_formula;
				}
				i3 = 0;
				if(new_formula.length() > i2 + 2 && new_formula[i2 + 2] == ID_WRAP_LEFT_CH) {
					if((i3 = new_formula.find(ID_WRAP_RIGHT_CH, i2 + 2)) != string::npos) {
						svar_v = new_formula.substr(i2 + 3, i3 - (i2 + 3));
						i3 -= i2 + 1;
					} else i3 = 0;
				}
				if(i3) {
					default_values.push_back(svar_v);
				} else {
					default_values.push_back("0");
				}
				new_formula.replace(i2, 2 + i3, svar);
				while((i2 = new_formula.find(svar_o, i2 + 1)) != string::npos) {
					if(i2 > 0 && new_formula[i2 - 1] == '\\') {
						i2++;
					} else {
						if(i2 + 4 < new_formula.length() && new_formula[i2 + 2] == ID_WRAP_LEFT_CH && (i3 = new_formula.find(ID_WRAP_RIGHT_CH, i2 + 3)) != string::npos) {
							new_formula.replace(i2, i3 - i2 + 1, svar);
						} else {
							new_formula.replace(i2, 2, svar);
						}
					}
				}
				for(size_t sub_i = 0; sub_i < priv->v_subs_calc.size(); sub_i++) {
					i2 = 0;
					while((i2 = priv->v_subs_calc[sub_i].find(svar_o, i2 + 1)) != string::npos) {
						if(i2 > 0 && priv->v_subs_calc[sub_i][i2 - 1] == '\\') {
							i2++;
						} else {
							priv->v_subs_calc[sub_i].replace(i2, 2, svar);
						}
					}
				}
				optionals = true;
			} else if((i2 = new_formula.find(svar, i5)) != string::npos) {
				if(i2 > 0 && new_formula[i2 - 1] == '\\') {
					i5 = i2 + 2;
					goto before_find_in_set_formula;
				}
			} else {
				b = false;
				for(size_t sub_i = 0; sub_i < priv->v_subs_calc.size(); sub_i++) {
					before_find_in_vsubs_set_formula:
					if(i < 24 && (i2 = priv->v_subs_calc[sub_i].find(svar_o, i4)) != string::npos) {
						if(i2 > 0 && priv->v_subs_calc[sub_i][i2 - 1] == '\\') {
							i4 = i2 + 2;
							goto before_find_in_vsubs_set_formula;
						}
						i3 = 0;
						if(priv->v_subs_calc[sub_i].length() > i2 + 2 && priv->v_subs_calc[sub_i][i2 + 2] == ID_WRAP_LEFT_CH) {
							if((i3 = priv->v_subs_calc[sub_i].find(ID_WRAP_RIGHT_CH, i2 + 2)) != string::npos) {
								svar_v = priv->v_subs_calc[sub_i].substr(i2 + 3, i3 - (i2 + 3));
								i3 -= i2 + 1;
							} else i3 = 0;
						}
						if(i3) {
							default_values.push_back(svar_v);
						} else {
							default_values.push_back("0");
						}
						priv->v_subs_calc[sub_i].replace(i2, 2 + i3, svar);
						while((i2 = priv->v_subs_calc[sub_i].find(svar_o, i2 + 1)) != string::npos) {
							if(i2 > 0 && priv->v_subs_calc[sub_i][i2 - 1] == '\\') {
								i2++;
							} else {
								priv->v_subs_calc[sub_i].replace(i2, 2, svar);
							}
						}
						optionals = true;
						b = true;
					} else if((i2 = priv->v_subs_calc[sub_i].find(svar, i5)) != string::npos) {
						if(i2 > 0 && priv->v_subs_calc[sub_i][i2 - 1] == '\\') {
							i5 = i2 + 2;
							goto before_find_in_vsubs_set_formula;
						}
						b = true;
					}
				}
				if(!b) {
					if(i < 24 && !optionals) {
						i = 24;
						goto begin_loop_in_set_formula;
					}
					break;
				}
			}
			if(i >= 24) {
				max_argc_ = -1;
			} else {
				if(optionals) {
					max_argc_++;
				} else {
					max_argc_++;
					argc_++;
				}
			}
		}
	}
	if(argc_ > 24) {
		argc_ = 24;
	}
	if(max_argc_ > 24) {
		max_argc_ = 24;
	}
	if(max_argc_ < 0 || argc_ < 0) {
		max_argc_ = -1;
		if(argc_ < 0) argc_ = 0;
	} else if(max_argc_ < argc_) {
		max_argc_ = argc_;
	}

	while((int) default_values.size() < max_argc_ - argc_) {
		default_values.push_back("0");
	}
	if(max_argc_ > 0) default_values.resize(max_argc_ - argc_);
	sformula_calc = new_formula;
	argc = argc_;
	max_argc = max_argc_;
}
void UserFunction::addSubfunction(string subfunction, bool precalculate) {
	setChanged(true);
	v_subs.push_back(subfunction);
	v_precalculate.push_back(precalculate);
}
void UserFunction::setSubfunction(size_t index, string subfunction) {
	if(index > 0 && index <= v_subs.size()) {
		setChanged(true);
		v_subs[index - 1] = subfunction;
	}
}
void UserFunction::delSubfunction(size_t index) {
	if(index > 0 && index <= v_subs.size()) {
		setChanged(true);
		v_subs.erase(v_subs.begin() + (index - 1));
		if(index <= priv->v_subs_calc.size()) priv->v_subs_calc.erase(priv->v_subs_calc.begin() + (index - 1));
	}
	if(index > 0 && index <= v_precalculate.size()) {
		setChanged(true);
		v_precalculate.erase(v_precalculate.begin() + (index - 1));
	}
}
void UserFunction::clearSubfunctions() {
	setChanged(true);
	v_subs.clear();
	priv->v_subs_calc.clear();
	v_precalculate.clear();
}
void UserFunction::setSubfunctionPrecalculated(size_t index, bool precalculate) {
	if(index > 0 && index <= v_precalculate.size()) {
		setChanged(true);
		v_precalculate[index - 1] = precalculate;
	}
}
size_t UserFunction::countSubfunctions() const {
	return v_subs.size();
}
const string &UserFunction::getSubfunction(size_t index) const {
	if(index > 0 && index <= v_subs.size()) {
		return v_subs[index - 1];
	}
	return empty_string;
}
bool UserFunction::subfunctionPrecalculated(size_t index) const {
	if(index > 0 && index <= v_precalculate.size()) {
		return v_precalculate[index - 1];
	}
	return false;
}

Argument::Argument(string name_, bool does_test, bool does_error) {
	sname = name_;
	remove_blank_ends(sname);
	scondition = "";
	b_zero = true;
	b_test = does_test;
	b_matrix = false;
	b_text = false;
	b_error = does_error;
	b_rational = false;
	b_last = false;
	b_handle_vector = false;
}
Argument::Argument(const Argument *arg) {
	b_text = false;
	set(arg);
}
Argument::~Argument() {}
Argument *Argument::copy() const {
	return new Argument(this);
}
string Argument::print() const {return "";}
string Argument::subprintlong() const {return _("a free value");}
string Argument::printlong() const {
	string str = subprintlong();
	if(!b_zero) {
		str += " ";
		str += _("that is nonzero");
	}
	if(b_rational) {
		if(!b_zero) {
			str += " ";
			str += _("and");
		}
		str += " ";
		str += _("that is rational (polynomial)");
	}
	if(!scondition.empty()) {
		if(!b_zero || b_rational) {
			str += " ";
			str += _("and");
		}
		str += " ";
		str += _("that fulfills the condition:");
		str += " \"";
		string str2 = CALCULATOR->localizeExpression(scondition);
		if(name().empty()) gsub("\\x", _("argument"), str2);
		else gsub("\\x", name(), str2);
		str += str2;
		str += "\"";
	}
	return str;
}
void Argument::set(const Argument *arg) {
	sname = arg->name();
	scondition = arg->getCustomCondition();
	b_zero = !arg->zeroForbidden();
	b_test = arg->tests();
	b_matrix = arg->matrixAllowed();
	b_rational = arg->rationalPolynomial();
	b_last = arg->isLastArgument();
	b_handle_vector = arg->handlesVector();
}
bool Argument::test(MathStructure &value, int index, MathFunction *f, const EvaluationOptions &eo) const {
	if(!b_test) {
		return true;
	}
	bool evaled = false;
	bool b = subtest(value, eo);
	if(b && !b_zero) {
		if(!value.isNumber() && !value.representsNonZero()) {
			value.eval(eo);
			evaled = true;
		}
		b = value.representsNonZero();
	}
	if(b && b_rational) {
		if(!evaled) {
			value.eval(eo);
			evaled = true;
		}
		b = value.isRationalPolynomial();
	}
	if(!b && b_matrix) {
		if(!evaled && !value.isMatrix()) {
			value.eval(eo);
			evaled = true;
		}
		b = value.isMatrix();
	}
	if(b && !scondition.empty()) {
		string expression = scondition;
		int id = CALCULATOR->addId(new MathStructure(value), true);
		string ids = LEFT_PARENTHESIS ID_WRAP_LEFT;
		ids += i2s(id);
		ids += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
		gsub("\\x", ids, expression);
		b = CALCULATOR->testCondition(expression);
		CALCULATOR->delId(id);
	}
	if(!b && b_handle_vector) {
		if(!evaled && !value.isVector()) {
			value.eval(eo);
			evaled = true;
		}
		if(value.isVector()) return false;
	}
	if(!b) {
		if(b_error && (type() != ARGUMENT_TYPE_SYMBOLIC || !value.isUndefined())) {
			if(sname.empty()) {
				CALCULATOR->error(true, _("Argument %s in %s() must be %s."), i2s(index).c_str(), f->name().c_str(), printlong().c_str(), NULL);
			} else {
				CALCULATOR->error(true, _("Argument %s, %s, in %s() must be %s."), i2s(index).c_str(), sname.c_str(), f->name().c_str(), printlong().c_str(), NULL);
			}
		}
		return false;
	}
	return true;
}
MathStructure Argument::parse(const string &str, const ParseOptions &po) const {
	MathStructure mstruct;
	parse(&mstruct, str, po);
	return mstruct;
}
void Argument::parse(MathStructure *mstruct, const string &str, const ParseOptions &po) const {
	if(b_text) {
		MathFunction *f_cat = CALCULATOR->getFunctionById(FUNCTION_ID_CONCATENATE);
		for(size_t i = 1; i <= f_cat->countNames(); i++) {
			if(str.find(f_cat->getName(i).name) != string::npos) {
				CALCULATOR->parse(mstruct, str, po);
				return;
			}
		}
		size_t pars = 0;
		while(true) {
			size_t pars2 = 1;
			size_t i = pars;
			if(str.length() >= 2 + pars * 2 && str[pars] == LEFT_PARENTHESIS_CH && str[str.length() - 1 - pars] == RIGHT_PARENTHESIS_CH) {
				while(true) {
					i = str.find_first_of(LEFT_PARENTHESIS RIGHT_PARENTHESIS, i + 1);
					if(i == string::npos || i >= str.length() - 1 - pars) {
						break;
					} else if(str[i] == LEFT_PARENTHESIS_CH) {
						pars2++;
					} else if(str[i] == RIGHT_PARENTHESIS_CH) {
						pars2--;
						if(pars2 == 0) {
							break;
						}
					}
				}
				if(pars2 > 0) {
					pars++;
				}
			} else {
				break;
			}
			if(pars2 == 0) break;
		}
		size_t cits = 0;
		if(str.length() >= 2 + pars * 2) {
			if(str[pars] == ID_WRAP_LEFT_CH && str[str.length() - 1 - pars] == ID_WRAP_RIGHT_CH && str.find(ID_WRAP_RIGHT, pars + 1) == str.length() - 1 - pars) {
				CALCULATOR->parse(mstruct, str.substr(pars, str.length() - pars * 2), po);
				if(mstruct->isDateTime() && !mstruct->datetime()->parsed_string.empty()) mstruct->set(mstruct->datetime()->parsed_string, false, true);
				return;
			}
			if(str[pars] == '\\' && str[str.length() - 1 - pars] == '\\') {
				CALCULATOR->parse(mstruct, str.substr(1 + pars, str.length() - 2 - pars * 2), po);
				if(mstruct->isDateTime() && !mstruct->datetime()->parsed_string.empty()) mstruct->set(mstruct->datetime()->parsed_string, false, true);
				return;
			}
			if((str[pars] == '\"' && str[str.length() - 1 - pars] == '\"') || (str[pars] == '\'' && str[str.length() - 1 - pars] == '\'')) {
				size_t i = pars + 1, cits = 0;
				while(i < str.length() - 1 - pars) {
					i = str.find(str[pars], i);
					if(i >= str.length() - 1 - pars) {
						break;
					}
					cits++;
					i++;
				}
			}
		}
		string str2;
		if(pars == 0 || (cits / 2) % 2 != 0) str2 = str;
		else str2 = str.substr(pars + (cits ? 1 : 0), str.length() - pars * 2 - (cits ? 2 : 0));
		if(cits == 0) replace_internal_operators(str2);
		if((cits / 2) % 2 == 0) {
			size_t i = str2.find(ID_WRAP_LEFT);
			if(i != string::npos && i < str2.length() - 2) {
				i = 0;
				size_t i2 = 0; int id = 0;
				while((i = str2.find(ID_WRAP_LEFT, i)) != string::npos) {
					i2 = str2.find(ID_WRAP_RIGHT, i + 1);
					if(i2 == string::npos) break;
					id = s2i(str2.substr(i + 1, i2 - (i + 1)));
					MathStructure *m_temp = CALCULATOR->getId((size_t) id);
					bool do_par = (i == 0 || i2 + 1 == str2.length() || str2[i - 1] != LEFT_PARENTHESIS_CH || str2[i2 + 1] != RIGHT_PARENTHESIS_CH);
					string str3;
					if(do_par) str3 = LEFT_PARENTHESIS_CH;
					if(!m_temp) {
						CALCULATOR->error(true, _("Internal id %s does not exist."), i2s(id).c_str(), NULL);
						str3 += CALCULATOR->getVariableById(VARIABLE_ID_UNDEFINED)->preferredInputName(true, false, false, true).name;
					} else {
						str3 += m_temp->print(CALCULATOR->save_printoptions).c_str();
						m_temp->unref();
					}
					if(do_par) str3 += RIGHT_PARENTHESIS_CH;
					str2.replace(i, i2 - i + 1, str3);
					i += str3.length();
				}
			}
		}
		mstruct->set(str2, false, true);
	} else {
		CALCULATOR->parse(mstruct, str, po);
	}
}

bool Argument::subtest(MathStructure&, const EvaluationOptions&) const {
	return true;
}
string Argument::name() const {
	return sname;
}
void Argument::setName(string name_) {
	sname = name_;
	remove_blank_ends(sname);
}
void Argument::setCustomCondition(string condition) {
	scondition = condition;
	remove_blank_ends(scondition);
}
string Argument::getCustomCondition() const {
	return scondition;
}

bool Argument::zeroForbidden() const {
	return !b_zero;
}
void Argument::setZeroForbidden(bool forbid_zero) {
	b_zero = !forbid_zero;
}

bool Argument::tests() const {
	return b_test;
}
void Argument::setTests(bool does_test) {
	b_test = does_test;
}

bool Argument::alerts() const {
	return b_error;
}
void Argument::setAlerts(bool does_error) {
	b_error = does_error;
}
bool Argument::suggestsQuotes() const {return b_text || type() == ARGUMENT_TYPE_DATE;}
int Argument::type() const {
	return ARGUMENT_TYPE_FREE;
}
bool Argument::matrixAllowed() const {return b_matrix;}
void Argument::setMatrixAllowed(bool allow_matrix) {b_matrix = allow_matrix;}
bool Argument::handlesVector() const {return b_handle_vector;}
void Argument::setHandleVector(bool handle_vector) {b_handle_vector = handle_vector;}
bool Argument::isLastArgument() const {return b_last;}
void Argument::setIsLastArgument(bool is_last) {b_last = is_last;}
bool Argument::rationalPolynomial() const {return b_rational;}
void Argument::setRationalPolynomial(bool rational_polynomial) {b_rational = rational_polynomial;}

NumberArgument::NumberArgument(string name_, ArgumentMinMaxPreDefinition minmax, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {
	fmin = NULL;
	fmax = NULL;
	b_incl_min = true;
	b_incl_max = true;
	b_complex = true;
	b_rational_number = false;
	switch(minmax) {
		case ARGUMENT_MIN_MAX_POSITIVE: {
			fmin = new Number();
			b_incl_min = false;
			break;
		}
		case ARGUMENT_MIN_MAX_NEGATIVE: {
			fmax = new Number();
			b_incl_max = false;
			break;
		}
		case ARGUMENT_MIN_MAX_NONNEGATIVE: {
			fmin = new Number();
			break;
		}
		case ARGUMENT_MIN_MAX_NONZERO: {
			setZeroForbidden(true);
			break;
		}
		default: {}
	}
	b_handle_vector = does_test;
}
NumberArgument::NumberArgument(const NumberArgument *arg) {
	fmin = NULL;
	fmax = NULL;
	set(arg);
}
NumberArgument::~NumberArgument() {
	if(fmin) {
		delete fmin;
	}
	if(fmax) {
		delete fmax;
	}
}

void NumberArgument::setMin(const Number *nmin) {
	if(!nmin) {
		if(fmin) {
			delete fmin;
		}
		return;
	}
	if(!fmin) {
		fmin = new Number(*nmin);
	} else {
		fmin->set(*nmin);
	}
}
void NumberArgument::setIncludeEqualsMin(bool include_equals) {
	b_incl_min = include_equals;
}
bool NumberArgument::includeEqualsMin() const {
	return b_incl_min;
}
const Number *NumberArgument::min() const {
	return fmin;
}
void NumberArgument::setMax(const Number *nmax) {
	if(!nmax) {
		if(fmax) {
			delete fmax;
		}
		return;
	}
	if(!fmax) {
		fmax = new Number(*nmax);
	} else {
		fmax->set(*nmax);
	}
}
void NumberArgument::setIncludeEqualsMax(bool include_equals) {
	b_incl_max = include_equals;
}
bool NumberArgument::includeEqualsMax() const {
	return b_incl_max;
}
const Number *NumberArgument::max() const {
	return fmax;
}
bool NumberArgument::complexAllowed() const {
	return b_complex;
}
void NumberArgument::setComplexAllowed(bool allow_complex) {
	b_complex = allow_complex;
}
bool NumberArgument::rationalNumber() const {
	return b_rational_number;
}
void NumberArgument::setRationalNumber(bool rational_number) {
	b_rational_number = rational_number;
}
bool NumberArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isNumber()) {
		value.eval(eo);
	}
	/*if(value.isMultiplication() && value.size() == 2 && value[0].isNumber() && value[1].isUnit() && value[1].unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit()) {
		value /= CALCULATOR->getRadUnit();
		value.eval(eo);
	}*/
	if(!value.isNumber() || (b_rational_number && !value.number().isRational())) {
		return false;
	}
	if(!b_complex && value.number().hasImaginaryPart()) {
		if(!value.number().imaginaryPartIsNonZero()) value.number().clearImaginary();
		else return false;
	}
	if(fmin) {
		ComparisonResult cmpr = fmin->compare(value.number());
		if(!(cmpr == COMPARISON_RESULT_GREATER || (b_incl_min && COMPARISON_IS_EQUAL_OR_GREATER(cmpr)))) {
			return false;
		}
	}
	if(fmax) {
		ComparisonResult cmpr = fmax->compare(value.number());
		if(!(cmpr == COMPARISON_RESULT_LESS || (b_incl_max && COMPARISON_IS_EQUAL_OR_LESS(cmpr)))) {
			return false;
		}
	}
	return true;
}
int NumberArgument::type() const {
	return ARGUMENT_TYPE_NUMBER;
}
Argument *NumberArgument::copy() const {
	return new NumberArgument(this);
}
void NumberArgument::set(const Argument *arg) {
	if(arg->type() == ARGUMENT_TYPE_NUMBER) {
		const NumberArgument *farg = (const NumberArgument*) arg;
		b_incl_min = farg->includeEqualsMin();
		b_incl_max = farg->includeEqualsMax();
		b_complex = farg->complexAllowed();
		b_rational_number = farg->rationalNumber();
		if(fmin) {
			delete fmin;
			fmin = NULL;
		}
		if(fmax) {
			delete fmax;
			fmax = NULL;
		}
		if(farg->min()) {
			fmin = new Number(*farg->min());
		}
		if(farg->max()) {
			fmax = new Number(*farg->max());
		}
	}
	Argument::set(arg);
}
string NumberArgument::print() const {
	return _("number");
}
string NumberArgument::subprintlong() const {
	string str;
	if(b_rational_number) {
		str += _("a rational number");
	} else if(b_complex) {
		str += _("a number");
	} else {
		str += _("a real number");
	}
	if(fmin) {
		str += " ";
		if(b_incl_min) {
			str += ">=";
		} else {
			str += ">";
		}
		str += " ";
		str += fmin->print();
	}
	if(fmax) {
		if(fmin) {
			str += " ";
			str += _("and");
		}
		str += " ";
		if(b_incl_max) {
			str += "<=";
		} else {
			str += "<";
		}
		str += " ";
		str += fmax->print();
	}
	return str;
}

IntegerArgument::IntegerArgument(string name_, ArgumentMinMaxPreDefinition minmax, bool does_test, bool does_error, IntegerType integer_type) : Argument(name_, does_test, does_error) {
	imin = NULL;
	imax = NULL;
	i_inttype = integer_type;
	switch(minmax) {
		case ARGUMENT_MIN_MAX_POSITIVE: {
			imin = new Number(1, 1);
			break;
		}
		case ARGUMENT_MIN_MAX_NEGATIVE: {
			imax = new Number(-1, 1);
			break;
		}
		case ARGUMENT_MIN_MAX_NONNEGATIVE: {
			imin = new Number();
			break;
		}
		case ARGUMENT_MIN_MAX_NONZERO: {
			setZeroForbidden(true);
			break;
		}
		default: {}
	}
	b_handle_vector = does_test;
}
IntegerArgument::IntegerArgument(const IntegerArgument *arg) {
	imin = NULL;
	imax = NULL;
	i_inttype = INTEGER_TYPE_NONE;
	set(arg);
}
IntegerArgument::~IntegerArgument() {
	if(imin) {
		delete imin;
	}
	if(imax) {
		delete imax;
	}
}

IntegerType IntegerArgument::integerType() const {return i_inttype;}
void IntegerArgument::setIntegerType(IntegerType integer_type) {i_inttype = integer_type;}
void IntegerArgument::setMin(const Number *nmin) {
	if(!nmin) {
		if(imin) {
			delete imin;
		}
		return;
	}
	if(!imin) {
		imin = new Number(*nmin);
	} else {
		imin->set(*nmin);
	}
}
const Number *IntegerArgument::min() const {
	return imin;
}
void IntegerArgument::setMax(const Number *nmax) {
	if(!nmax) {
		if(imax) {
			delete imax;
		}
		return;
	}
	if(!imax) {
		imax = new Number(*nmax);
	} else {
		imax->set(*nmax);
	}
}
const Number *IntegerArgument::max() const {
	return imax;
}
bool IntegerArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isNumber()) {
		value.eval(eo);
	}
	if(!value.isNumber() || !value.number().isInteger(i_inttype)) {
		return false;
	}
	if(imin) {
		ComparisonResult cmpr = imin->compare(value.number());
		if(!(COMPARISON_IS_EQUAL_OR_GREATER(cmpr))) {
			return false;
		}
	}
	if(imax) {
		ComparisonResult cmpr = imax->compare(value.number());
		if(!(COMPARISON_IS_EQUAL_OR_LESS(cmpr))) {
			return false;
		}
	}
	return true;
}
int IntegerArgument::type() const {
	return ARGUMENT_TYPE_INTEGER;
}
Argument *IntegerArgument::copy() const {
	return new IntegerArgument(this);
}
void IntegerArgument::set(const Argument *arg) {
	if(arg->type() == ARGUMENT_TYPE_INTEGER) {
		const IntegerArgument *iarg = (const IntegerArgument*) arg;
		if(imin) {
			delete imin;
			imin = NULL;
		}
		if(imax) {
			delete imax;
			imax = NULL;
		}
		if(iarg->min()) {
			imin = new Number(*iarg->min());
		}
		if(iarg->max()) {
			imax = new Number(*iarg->max());
		}
		i_inttype = iarg->integerType();
	}
	Argument::set(arg);
}
string IntegerArgument::print() const {
	return _("integer");
}
string IntegerArgument::subprintlong() const {
	string str = _("an integer");
	if(imin) {
		str += " ";
		str += ">=";
		str += " ";
		str += imin->print();
	} else if(i_inttype != INTEGER_TYPE_NONE) {
		str += " ";
		str += ">=";
		str += " ";
		switch(i_inttype) {
			case INTEGER_TYPE_SIZE: {}
			case INTEGER_TYPE_UINT: {str += "0"; break;}
			case INTEGER_TYPE_SINT: {str += i2s(INT_MIN); break;}
			case INTEGER_TYPE_ULONG: {str += "0"; break;}
			case INTEGER_TYPE_SLONG: {str += i2s(LONG_MIN); break;}
			default: {}
		}
	}
	if(imax) {
		if(imin || i_inttype != INTEGER_TYPE_NONE) {
			str += " ";
			str += _("and");
		}
		str += " ";
		str += "<=";
		str += " ";
		str += imax->print();
	} else if(i_inttype != INTEGER_TYPE_NONE) {
		str += " ";
		str += _("and");
		str += " ";
		str += "<=";
		str += " ";
		switch(i_inttype) {
			case INTEGER_TYPE_SIZE: {}
			case INTEGER_TYPE_UINT: {str += u2s(UINT_MAX); break;}
			case INTEGER_TYPE_SINT: {str += i2s(INT_MAX); break;}
			case INTEGER_TYPE_ULONG: {str += u2s(ULONG_MAX); break;}
			case INTEGER_TYPE_SLONG: {str += i2s(LONG_MAX); break;}
			default: {}
		}
	}
	return str;
}

SymbolicArgument::SymbolicArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {}
SymbolicArgument::SymbolicArgument(const SymbolicArgument *arg) {set(arg);}
SymbolicArgument::~SymbolicArgument() {}
bool SymbolicArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic() && (!value.isVariable() || value.variable()->isKnown())) {
		value.eval(eo);
	}
	return value.isSymbolic() || (value.isVariable() && !value.variable()->isKnown());
}
int SymbolicArgument::type() const {return ARGUMENT_TYPE_SYMBOLIC;}
Argument *SymbolicArgument::copy() const {return new SymbolicArgument(this);}
string SymbolicArgument::print() const {return _("symbol");}
string SymbolicArgument::subprintlong() const {return _("an unknown variable/symbol");}

TextArgument::TextArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
TextArgument::TextArgument(const TextArgument *arg) {set(arg); b_text = true;}
TextArgument::~TextArgument() {}
bool TextArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic();
}
int TextArgument::type() const {return ARGUMENT_TYPE_TEXT;}
Argument *TextArgument::copy() const {return new TextArgument(this);}
string TextArgument::print() const {return _("text");}
string TextArgument::subprintlong() const {return _("a text string");}
bool TextArgument::suggestsQuotes() const {return b_text;}

DateArgument::DateArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {}
DateArgument::DateArgument(const DateArgument *arg) {set(arg);}
DateArgument::~DateArgument() {}
void DateArgument::parse(MathStructure *mstruct, const string &str, const ParseOptions &po) const {
	QalculateDateTime dt_test;
	if(dt_test.set(str)) {
		mstruct->set(dt_test);
	} else {
		Argument::parse(mstruct, str, po);
	}
}
bool DateArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isDateTime()) {
		value.eval(eo);
	}
	return value.isDateTime();
}
int DateArgument::type() const {return ARGUMENT_TYPE_DATE;}
Argument *DateArgument::copy() const {return new DateArgument(this);}
string DateArgument::print() const {return string(_("date")) + " (Y-M-D)";}
string DateArgument::subprintlong() const {return string(_("a date")) + " (Y-M-D)";}

VectorArgument::VectorArgument(string name_, bool does_test, bool allow_matrix, bool does_error) : Argument(name_, does_test, does_error) {
	setMatrixAllowed(allow_matrix);
	b_argloop = true;
}
VectorArgument::VectorArgument(const VectorArgument *arg) {
	set(arg);
	b_argloop = arg->reoccuringArguments();
	size_t i = 1;
	while(true) {
		if(!arg->getArgument(i)) break;
		subargs.push_back(arg->getArgument(i)->copy());
		i++;
	}
}
VectorArgument::~VectorArgument() {
	for(size_t i = 0; i < subargs.size(); i++) {
		delete subargs[i];
	}
}
bool VectorArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	bool subvector = false;
	for(size_t i = 0; i < subargs.size(); i++) {
		if(subargs[i] && (subargs[i]->type() == ARGUMENT_TYPE_VECTOR || subargs[i]->type() == ARGUMENT_TYPE_MATRIX)) {
			subvector = true;
			break;
		}
	}
	if(subvector && value.isFunction() && (value.function()->id() == FUNCTION_ID_HORZCAT || value.function()->id() == FUNCTION_ID_VERTCAT)) {
		value.setType(STRUCT_VECTOR);
	}
	if(!subvector || !value.isVector()) {
		value.eval(eo);
	}
	if(!value.isVector()) {
		if(isLastArgument() && eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES) value.transform(STRUCT_VECTOR);
		else if(value.representsScalar()) value.transform(STRUCT_VECTOR);
		else return false;
	}
	if(value.isMatrix() && value.columns() == 1 && value.rows() > 1) {
		value.transposeMatrix();
	}
	if(b_argloop && subargs.size() > 0) {
		for(size_t i = 0; i < value.countChildren(); i++) {
			if(!subargs[i % subargs.size()]->test(value[i], 1, NULL, eo)) {
				return false;
			}
		}
	} else {
		for(size_t i = 0; i < subargs.size() && i < value.countChildren(); i++) {
			if(!subargs[i]->test(value[i], 1, NULL, eo)) {
				return false;
			}
		}
	}
	return true;
}
int VectorArgument::type() const {return ARGUMENT_TYPE_VECTOR;}
Argument *VectorArgument::copy() const {return new VectorArgument(this);}
string VectorArgument::print() const {return _("vector");}
string VectorArgument::subprintlong() const {
	if(subargs.size() > 0) {
		string str = _("a vector with ");
		for(size_t i = 0; i < subargs.size(); i++) {
			if(i > 0) {
				str += ", ";
			}
			str += subargs[i]->printlong();
		}
		if(b_argloop) {
			str += ", ...";
		}
		return str;
	} else {
		return _("a vector");
	}
}
bool VectorArgument::reoccuringArguments() const {
	return b_argloop;
}
void VectorArgument::setReoccuringArguments(bool reocc) {
	b_argloop = reocc;
}
void VectorArgument::addArgument(Argument *arg) {
	arg->setAlerts(false);
	subargs.push_back(arg);
}
void VectorArgument::delArgument(size_t index) {
	if(index > 0 && index <= subargs.size()) {
		subargs.erase(subargs.begin() + (index - 1));
	}
}
size_t VectorArgument::countArguments() const {
	return subargs.size();
}
Argument *VectorArgument::getArgument(size_t index) const {
	if(index > 0 && index <= subargs.size()) {
		return subargs[index - 1];
	}
	return NULL;
}

MatrixArgument::MatrixArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {
	b_square = false;
}
MatrixArgument::MatrixArgument(const MatrixArgument *arg) {
	set(arg);
	b_square = arg->squareDemanded();
}
MatrixArgument::~MatrixArgument() {}
bool MatrixArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	value.eval(eo);
	if(!value.isMatrix()) {
		if(value.isVector() && (value.size() == 0 || value[0].representsScalar())) {
			if(CALCULATOR->usesMatlabStyleMatrices() || value.size() == 0) {
				value.transform(STRUCT_VECTOR);
			} else {
				for(size_t i = 0; i < value.size(); i++) {
					value[i].transform(STRUCT_VECTOR);
				}
			}
		} else if(value.representsScalar()) {
			value.transform(STRUCT_VECTOR);
			value.transform(STRUCT_VECTOR);
		}
	}
	return value.isMatrix() && (!b_square || value.matrixIsSquare());
}
bool MatrixArgument::squareDemanded() const {return b_square;}
void MatrixArgument::setSquareDemanded(bool square) {b_square = square;}
int MatrixArgument::type() const {return ARGUMENT_TYPE_MATRIX;}
Argument *MatrixArgument::copy() const {return new MatrixArgument(this);}
string MatrixArgument::print() const {return _("matrix");}
string MatrixArgument::subprintlong() const {
	if(b_square) {
		return _("a square matrix");
	} else {
		return _("a matrix");
	}
}

ExpressionItemArgument::ExpressionItemArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
ExpressionItemArgument::ExpressionItemArgument(const ExpressionItemArgument *arg) {set(arg); b_text = true;}
ExpressionItemArgument::~ExpressionItemArgument() {}
bool ExpressionItemArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic() && CALCULATOR->getExpressionItem(value.symbol());
}
int ExpressionItemArgument::type() const {return ARGUMENT_TYPE_EXPRESSION_ITEM;}
Argument *ExpressionItemArgument::copy() const {return new ExpressionItemArgument(this);}
string ExpressionItemArgument::print() const {return _("object");}
string ExpressionItemArgument::subprintlong() const {return _("a valid function, unit or variable name");}

FunctionArgument::FunctionArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
FunctionArgument::FunctionArgument(const FunctionArgument *arg) {set(arg); b_text = true;}
FunctionArgument::~FunctionArgument() {}
bool FunctionArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic() && CALCULATOR->getActiveFunction(value.symbol());
}
int FunctionArgument::type() const {return ARGUMENT_TYPE_FUNCTION;}
Argument *FunctionArgument::copy() const {return new FunctionArgument(this);}
string FunctionArgument::print() const {return _("function");}
string FunctionArgument::subprintlong() const {return _("a valid function name");}

UnitArgument::UnitArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
UnitArgument::UnitArgument(const UnitArgument *arg) {set(arg); b_text = true;}
UnitArgument::~UnitArgument() {}
bool UnitArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic() && CALCULATOR->getActiveUnit(value.symbol());
}
int UnitArgument::type() const {return ARGUMENT_TYPE_UNIT;}
Argument *UnitArgument::copy() const {return new UnitArgument(this);}
string UnitArgument::print() const {return _("unit");}
string UnitArgument::subprintlong() const {return _("a valid unit name");}

VariableArgument::VariableArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
VariableArgument::VariableArgument(const VariableArgument *arg) {set(arg); b_text = true;}
VariableArgument::~VariableArgument() {}
bool VariableArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic() && CALCULATOR->getActiveVariable(value.symbol());
}
int VariableArgument::type() const {return ARGUMENT_TYPE_VARIABLE;}
Argument *VariableArgument::copy() const {return new VariableArgument(this);}
string VariableArgument::print() const {return _("variable");}
string VariableArgument::subprintlong() const {return _("a valid variable name");}

FileArgument::FileArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {b_text = true;}
FileArgument::FileArgument(const FileArgument *arg) {set(arg); b_text = true;}
FileArgument::~FileArgument() {}
bool FileArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic();
}
int FileArgument::type() const {return ARGUMENT_TYPE_FILE;}
Argument *FileArgument::copy() const {return new FileArgument(this);}
string FileArgument::print() const {return _("file");}
string FileArgument::subprintlong() const {return _("a valid file name");}

BooleanArgument::BooleanArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {}
BooleanArgument::BooleanArgument(const BooleanArgument *arg) {set(arg);}
BooleanArgument::~BooleanArgument() {}
bool BooleanArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isNumber()) {
		value.eval(eo);
	}
	return value.isZero() || value.isOne();
}
int BooleanArgument::type() const {return ARGUMENT_TYPE_BOOLEAN;}
Argument *BooleanArgument::copy() const {return new BooleanArgument(this);}
string BooleanArgument::print() const {return _("boolean");}
string BooleanArgument::subprintlong() const {return _("a boolean (0 or 1)");}

AngleArgument::AngleArgument(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {}
AngleArgument::AngleArgument(const AngleArgument *arg) {set(arg);}
AngleArgument::~AngleArgument() {}
bool AngleArgument::subtest(MathStructure&, const EvaluationOptions&) const {
	return true;
}
int AngleArgument::type() const {return ARGUMENT_TYPE_ANGLE;}
Argument *AngleArgument::copy() const {return new AngleArgument(this);}
string AngleArgument::print() const {return _("angle");}
string AngleArgument::subprintlong() const {return _("an angle or a number (using the default angle unit)");}
void AngleArgument::parse(MathStructure *mstruct, const string &str, const ParseOptions &po) const {
	CALCULATOR->parse(mstruct, str, po);
	if(po.angle_unit != ANGLE_UNIT_NONE) {
		if(mstruct->contains(CALCULATOR->getRadUnit(), false, true, true) > 0) return;
		if(mstruct->contains(CALCULATOR->getDegUnit(), false, true, true) > 0) return;
		if(mstruct->contains(CALCULATOR->getGraUnit(), false, true, true) > 0) return;
		if(contains_angle_unit(*mstruct, po)) return;
	}
	switch(po.angle_unit) {
		case ANGLE_UNIT_DEGREES: {
			mstruct->multiply(CALCULATOR->getDegUnit());
			break;
		}
		case ANGLE_UNIT_GRADIANS: {
			mstruct->multiply(CALCULATOR->getGraUnit());
			break;
		}
		case ANGLE_UNIT_RADIANS: {
			mstruct->multiply(CALCULATOR->getRadUnit());
			break;
		}
		default: {}
	}
}

ArgumentSet::ArgumentSet(string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {
}
ArgumentSet::ArgumentSet(const ArgumentSet *arg) {
	set(arg);
	size_t i = 1;
	while(true) {
		if(!arg->getArgument(i)) break;
		subargs.push_back(arg->getArgument(i)->copy());
		i++;
	}
}
ArgumentSet::~ArgumentSet() {
	for(size_t i = 0; i < subargs.size(); i++) {
		delete subargs[i];
	}
}
bool ArgumentSet::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	for(size_t i = 0; i < subargs.size(); i++) {
		if((i == 0 || subargs[i]->type() != ARGUMENT_TYPE_MATRIX || !subargs[i]->tests() || !value.representsScalar()) && subargs[i]->test(value, 1, NULL, eo)) {
			return true;
		}
	}
	return false;
}
int ArgumentSet::type() const {return ARGUMENT_TYPE_SET;}
Argument *ArgumentSet::copy() const {return new ArgumentSet(this);}
string ArgumentSet::print() const {
	string str = "";
	for(size_t i = 0; i < subargs.size(); i++) {
		if(i > 0) {
			if(i == subargs.size() - 1) {
				str += " ";
				str += _("or");
				str += " ";
			} else {
				str += ", ";
			}
		}
		str += subargs[i]->print();
	}
	return str;
}
string ArgumentSet::subprintlong() const {
	string str = "";
	for(size_t i = 0; i < subargs.size(); i++) {
		if(i > 0) {
			if(i == subargs.size() - 1) {
				str += " ";
				str += _("or");
				str += " ";
			} else {
				str += ", ";
			}
		}
		str += subargs[i]->printlong();
	}
	return str;
}
void ArgumentSet::addArgument(Argument *arg) {
	arg->setAlerts(false);
	subargs.push_back(arg);
}
void ArgumentSet::delArgument(size_t index) {
	if(index > 0 && index <= subargs.size()) {
		subargs.erase(subargs.begin() + (index - 1));
	}
}
size_t ArgumentSet::countArguments() const {
	return subargs.size();
}
Argument *ArgumentSet::getArgument(size_t index) const {
	if(index > 0 && index <= subargs.size()) {
		return subargs[index - 1];
	}
	return NULL;
}
