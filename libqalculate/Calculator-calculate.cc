/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include "QalculateDateTime.h"

#include <locale.h>
#ifdef _MSC_VER
#	include <sys/utime.h>
#else
#	include <unistd.h>
#	include <utime.h>
#endif
#include <time.h>
#include <sys/types.h>

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

#include "Calculator_p.h"

void autoConvert(const MathStructure &morig, MathStructure &mconv, const EvaluationOptions &eo) {
	if(!morig.containsType(STRUCT_UNIT, true)) {
		if(&mconv != &morig) mconv.set(morig);
		return;
	}
	switch(eo.auto_post_conversion) {
		case POST_CONVERSION_OPTIMAL: {
			mconv.set(CALCULATOR->convertToOptimalUnit(morig, eo, false));
			break;
		}
		case POST_CONVERSION_BASE: {
			mconv.set(CALCULATOR->convertToBaseUnits(morig, eo));
			break;
		}
		case POST_CONVERSION_OPTIMAL_SI: {
			mconv.set(CALCULATOR->convertToOptimalUnit(morig, eo, true));
			break;
		}
		default: {
			if(&mconv != &morig) mconv.set(morig);
		}
	}
	if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mconv.set(CALCULATOR->convertToMixedUnits(mconv, eo));
}

void CalculateThread::run() {
	enableAsynchronousCancel();
	while(true) {
		bool b_parse = true;
		if(!read<bool>(&b_parse)) break;
		void *x = NULL;
		if(!read<void *>(&x) || !x) break;
		MathStructure *mstruct = (MathStructure*) x;
		CALCULATOR->startControl();
		if(b_parse) {
			mstruct->setAborted();
			if(CALCULATOR->tmp_parsedstruct) CALCULATOR->tmp_parsedstruct->setAborted();
			//if(CALCULATOR->tmp_tostruct) CALCULATOR->tmp_tostruct->setUndefined();
			if(CALCULATOR->expression_to_calculate.find_first_of(ID_WRAPS) != string::npos) {
				string str = CALCULATOR->expression_to_calculate;
				bool quote1 = false, quote2 = false;
				size_t id_li = string::npos;
				for(size_t i = 0; i < str.size(); i++) {
					if(!quote1 && str[i] == '\'') {
						quote2 = !quote2;
						id_li = string::npos;
					} else if(!quote2 && str[i] == '\"') {
						quote1 = !quote1;
						id_li = string::npos;
					} else if(str[i] == ID_WRAP_LEFT_CH) {
						if(!quote2 && !quote1) str[i] = LEFT_PARENTHESIS_CH;
						else id_li = i;
					} else if(str[i] == ID_WRAP_RIGHT_CH) {
						if(!quote2 && !quote1) {
							str[i] = RIGHT_PARENTHESIS_CH;
						} else if(id_li != string::npos) {
							if(id_li < i - 1 && str.find_first_not_of(NUMBERS SPACES, id_li + 1) == i) {
								str[i] = RIGHT_PARENTHESIS_CH;
								str[id_li] = LEFT_PARENTHESIS_CH;
							}
							id_li = string::npos;
						}
					}
				}
				mstruct->set(CALCULATOR->calculate(str, CALCULATOR->tmp_evaluationoptions, CALCULATOR->tmp_parsedstruct, CALCULATOR->tmp_tostruct, CALCULATOR->tmp_maketodivision));
			} else {
				mstruct->set(CALCULATOR->calculate(CALCULATOR->expression_to_calculate, CALCULATOR->tmp_evaluationoptions, CALCULATOR->tmp_parsedstruct, CALCULATOR->tmp_tostruct, CALCULATOR->tmp_maketodivision));
			}
		} else {
			MathStructure meval(*mstruct);
			mstruct->setAborted();
			mstruct->set(CALCULATOR->calculate(meval, CALCULATOR->tmp_evaluationoptions, CALCULATOR->tmp_tostruct ? CALCULATOR->tmp_tostruct->symbol() : ""));
		}
		switch(CALCULATOR->tmp_proc_command) {
			case PROC_RPN_ADD: {
				CALCULATOR->RPNStackEnter(mstruct, false);
				break;
			}
			case PROC_RPN_SET: {
				CALCULATOR->setRPNRegister(CALCULATOR->tmp_rpnindex, mstruct, false);
				break;
			}
			case PROC_RPN_OPERATION_1: {
				if(CALCULATOR->RPNStackSize() > 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_RPN_OPERATION_2: {
				if(CALCULATOR->RPNStackSize() > 1) {
					CALCULATOR->deleteRPNRegister(1);
				}
				if(CALCULATOR->RPNStackSize() > 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_RPN_OPERATION_F: {
				for(size_t i = 0; (CALCULATOR->tmp_proc_registers < 0 || (int) i < CALCULATOR->tmp_proc_registers - 1) && CALCULATOR->RPNStackSize() > 1; i++) {
					CALCULATOR->deleteRPNRegister(1);
				}
				if(CALCULATOR->RPNStackSize() > 0 && CALCULATOR->tmp_proc_registers != 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_NO_COMMAND: {}
		}
		CALCULATOR->stopControl();
		CALCULATOR->b_busy = false;
	}
}
void Calculator::saveState() {
}
void Calculator::restoreState() {
}
void Calculator::clearBuffers() {
	unordered_map<size_t, bool>::iterator it = priv->ids_p.begin();
	while(it != priv->ids_p.end()) {
		if(!it->second) {
			priv->freed_ids.push_back(it->first);
			priv->id_structs.erase(it->first);
			priv->ids_ref.erase(it->first);
			priv->ids_p.erase(it);
		} else {
			++it;
		}
	}
	if(priv->id_structs.empty()) {
		priv->ids_i = 0;
		priv->freed_ids.clear();
	}
}
bool Calculator::abort() {
	i_aborted = 1;
	if(!b_busy) return true;
	if(!calculate_thread->running) {
		b_busy = false;
	} else {
		// wait 5 seconds for clean abortation
		int msecs = i_precision > 1000 ? 10000 : 5000;
		while(b_busy && msecs > 0) {
			sleep_ms(10);
			msecs -= 10;
		}
		if(b_busy) {

			// force thread cancellation
			calculate_thread->cancel();
			stopControl();

			// clean up
			stopped_messages_count.clear();
			stopped_warnings_count.clear();
			stopped_errors_count.clear();
			stopped_messages.clear();
			disable_errors_ref = 0;
			i_stop_interval = 0;
			i_start_interval = 0;
			if(tmp_rpn_mstruct) tmp_rpn_mstruct->unref();
			tmp_rpn_mstruct = NULL;

			// thread cancellation is not safe
			if(i_precision > 10000) {
				error(true, _("The calculation has been forcibly terminated. Please restart the application."), NULL);
			} else {
				error(true, _("The calculation has been forcibly terminated. Please restart the application and report this as a bug."), NULL);
			}

			b_busy = false;
			calculate_thread->start();
			return false;
		}
	}
	return true;
}
bool Calculator::busy() {
	return b_busy;
}
void Calculator::terminateThreads() {
	if(calculate_thread->running) {
		if(!calculate_thread->write(false) || !calculate_thread->write(NULL)) calculate_thread->cancel();
		for(size_t i = 0; i < 10 && calculate_thread->running; i++) {
			sleep_ms(1);
		}
		if(calculate_thread->running) calculate_thread->cancel();
	}
}
bool Calculator::calculateRPNRegister(size_t index, int msecs, const EvaluationOptions &eo) {
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(new MathStructure(*rpn_stack[rpn_stack.size() - index]), PROC_RPN_SET, index, msecs, eo);
}

bool Calculator::calculateRPN(MathStructure *mstruct, int command, size_t index, int msecs, const EvaluationOptions &eo, int function_arguments) {
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	tmp_evaluationoptions = eo;
	tmp_proc_command = command;
	tmp_rpnindex = index;
	tmp_rpn_mstruct = mstruct;
	tmp_proc_registers = function_arguments;
	tmp_tostruct = NULL;
	if(!calculate_thread->write(false)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		abort();
		return false;
	}
	return true;
}
bool Calculator::calculateRPN(string str, int command, size_t index, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division, int function_arguments) {
	MathStructure *mstruct = new MathStructure();
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	expression_to_calculate = str;
	tmp_evaluationoptions = eo;
	tmp_proc_command = command;
	tmp_rpnindex = index;
	tmp_rpn_mstruct = mstruct;
	tmp_parsedstruct = parsed_struct;
	tmp_tostruct = to_struct;
	tmp_maketodivision = make_to_division;
	tmp_proc_registers = function_arguments;
	if(!calculate_thread->write(true)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		abort();
		return false;
	}
	return true;
}

bool Calculator::calculateRPN(MathOperation op, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->add(m_zero, op);
		if(parsed_struct) parsed_struct->clear();
	} else if(rpn_stack.size() == 1) {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack.back());
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_INVERSE);
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		if(op == OPERATION_SUBTRACT) {
			mstruct = new MathStructure();
		} else if(op == OPERATION_DIVIDE) {
			mstruct = new MathStructure(1, 1, 0);
		} else {
			mstruct = new MathStructure(*rpn_stack.back());
		}
		mstruct->add(*rpn_stack.back(), op);
	} else {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack[rpn_stack.size() - 2]);
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure(*rpn_stack[rpn_stack.size() - 2]);
		mstruct->add(*rpn_stack.back(), op);
	}
	return calculateRPN(mstruct, PROC_RPN_OPERATION_2, 0, msecs, eo);
}
bool Calculator::calculateRPN(MathFunction *f, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct = new MathStructure(f, NULL);
	int iregs = 0;
	if(f->args() != 0) {
		size_t i = f->minargs();
		if(f == f_logn && rpn_stack.size() > 1) i = 2;
		bool fill_vector = (i > 0 && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_VECTOR);
		if(fill_vector && rpn_stack.size() < i) fill_vector = false;
		if(fill_vector && rpn_stack.size() > 0 && rpn_stack.back()->isVector()) fill_vector = false;
		if(fill_vector) {
			i = rpn_stack.size();
		} else if(i < 1) {
			i = 1;
		}
		for(; i > 0; i--) {
			if(i > rpn_stack.size()) {
				error(false, _("Stack is empty. Filling remaining function arguments with zeroes."), NULL);
				mstruct->addChild(m_zero);
			} else {
				if(fill_vector && rpn_stack.size() - i == (size_t) f->minargs() - 1) mstruct->addChild(m_empty_vector);
				if(fill_vector && rpn_stack.size() - i >= (size_t) f->minargs() - 1) mstruct->getChild(f->minargs())->addChild(*rpn_stack[rpn_stack.size() - i]);
				else mstruct->addChild(*rpn_stack[rpn_stack.size() - i]);
				iregs++;
			}
			if(!fill_vector && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_ANGLE) {
				Unit *u = default_angle_unit(eo);
				if(u) (*mstruct)[i - 1].multiply(u);
			}
		}
		if(fill_vector) mstruct->childrenUpdated();
		f->appendDefaultValues(*mstruct);
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	return calculateRPN(mstruct, PROC_RPN_OPERATION_F, 0, msecs, eo, iregs);
}
bool Calculator::calculateRPNBitwiseNot(int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setBitwiseNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setBitwiseNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	return calculateRPN(mstruct, PROC_RPN_OPERATION_1, 0, msecs, eo);
}
bool Calculator::calculateRPNLogicalNot(int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setLogicalNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setLogicalNot();
	}
	if(parsed_struct) parsed_struct->set(*rpn_stack.back());
	return calculateRPN(mstruct, PROC_RPN_OPERATION_1, 0, msecs, eo);
}
MathStructure *Calculator::calculateRPN(MathOperation op, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->add(m_zero, op);
		if(parsed_struct) parsed_struct->clear();
	} else if(rpn_stack.size() == 1) {
		if(parsed_struct) {
			parsed_struct->clear();
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure();
		mstruct->add(*rpn_stack.back(), op);
	} else {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack[rpn_stack.size() - 2]);
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure(*rpn_stack[rpn_stack.size() - 2]);
		mstruct->add(*rpn_stack.back(), op);
	}
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() > 1) {
		rpn_stack.back()->unref();
		rpn_stack.erase(rpn_stack.begin() + (rpn_stack.size() - 1));
	}
	if(rpn_stack.size() > 0) {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	} else {
		rpn_stack.push_back(mstruct);
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPN(MathFunction *f, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct = new MathStructure(f, NULL);
	size_t iregs = 0;
	if(f->args() != 0) {
		size_t i = f->minargs();
		bool fill_vector = (i > 0 && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_VECTOR);
		if(fill_vector && rpn_stack.size() < i) fill_vector = false;
		if(fill_vector && rpn_stack.size() > 0 && rpn_stack.back()->isVector()) fill_vector = false;
		if(fill_vector) {
			i = rpn_stack.size();
		} else if(i < 1) {
			i = 1;
		}
		for(; i > 0; i--) {
			if(i > rpn_stack.size()) {
				error(false, _("Stack is empty. Filling remaining function arguments with zeroes."), NULL);
				mstruct->addChild(m_zero);
			} else {
				if(fill_vector && rpn_stack.size() - i == (size_t) f->minargs() - 1) mstruct->addChild(m_empty_vector);
				if(fill_vector && rpn_stack.size() - i >= (size_t) f->minargs() - 1) mstruct->getChild(f->minargs())->addChild(*rpn_stack[rpn_stack.size() - i]);
				else mstruct->addChild(*rpn_stack[rpn_stack.size() - i]);
				iregs++;
			}
			if(!fill_vector && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_ANGLE) {
				Unit *u = default_angle_unit(eo);
				if(u) (*mstruct)[i - 1].multiply(u);
			}
		}
		if(fill_vector) mstruct->childrenUpdated();
		f->appendDefaultValues(*mstruct);
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(iregs == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		for(size_t i = 0; i < iregs - 1 && rpn_stack.size() > 1; i++) {
			rpn_stack.back()->unref();
			rpn_stack.pop_back();
			deleteRPNRegister(1);
		}
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPNBitwiseNot(const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setBitwiseNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setBitwiseNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPNLogicalNot(const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setLogicalNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setLogicalNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
bool Calculator::RPNStackEnter(MathStructure *mstruct, int msecs, const EvaluationOptions &eo) {
	return calculateRPN(mstruct, PROC_RPN_ADD, 0, msecs, eo);
}
bool Calculator::RPNStackEnter(string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	remove_blank_ends(str);
	if(str.empty() && rpn_stack.size() > 0) {
		rpn_stack.push_back(new MathStructure(*rpn_stack.back()));
		return true;
	}
	return calculateRPN(str, PROC_RPN_ADD, 0, msecs, eo, parsed_struct, to_struct, make_to_division);
}
void Calculator::RPNStackEnter(MathStructure *mstruct, bool eval, const EvaluationOptions &eo) {
	if(eval) {
		current_stage = MESSAGE_STAGE_CALCULATION;
		mstruct->eval(eo);
		current_stage = MESSAGE_STAGE_CONVERSION;
		autoConvert(*mstruct, *mstruct, eo);
		current_stage = MESSAGE_STAGE_UNSET;
	}
	rpn_stack.push_back(mstruct);
}
void Calculator::RPNStackEnter(string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	remove_blank_ends(str);
	if(str.empty() && rpn_stack.size() > 0) rpn_stack.push_back(new MathStructure(*rpn_stack.back()));
	else rpn_stack.push_back(new MathStructure(calculate(str, eo, parsed_struct, to_struct, make_to_division)));
}
bool Calculator::setRPNRegister(size_t index, MathStructure *mstruct, int msecs, const EvaluationOptions &eo) {
	if(mstruct == NULL) {
		deleteRPNRegister(index);
		return true;
	}
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(mstruct, PROC_RPN_SET, index, msecs, eo);
}
bool Calculator::setRPNRegister(size_t index, string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(str, PROC_RPN_SET, index, msecs, eo, parsed_struct, to_struct, make_to_division);
}
void Calculator::setRPNRegister(size_t index, MathStructure *mstruct, bool eval, const EvaluationOptions &eo) {
	if(mstruct == NULL) {
		deleteRPNRegister(index);
		return;
	}
	if(eval) {
		current_stage = MESSAGE_STAGE_CALCULATION;
		mstruct->eval(eo);
		current_stage = MESSAGE_STAGE_CONVERSION;
		autoConvert(*mstruct, *mstruct, eo);
		current_stage = MESSAGE_STAGE_UNSET;
	}
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	rpn_stack[index]->unref();
	rpn_stack[index] = mstruct;
}
void Calculator::setRPNRegister(size_t index, string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	MathStructure *mstruct = new MathStructure(calculate(str, eo, parsed_struct, to_struct, make_to_division));
	rpn_stack[index]->unref();
	rpn_stack[index] = mstruct;
}
void Calculator::deleteRPNRegister(size_t index) {
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	rpn_stack[index]->unref();
	rpn_stack.erase(rpn_stack.begin() + index);
}
MathStructure *Calculator::getRPNRegister(size_t index) const {
	if(index > 0 && index <= rpn_stack.size()) {
		index = rpn_stack.size() - index;
		return rpn_stack[index];
	}
	return NULL;
}
size_t Calculator::RPNStackSize() const {
	return rpn_stack.size();
}
void Calculator::clearRPNStack() {
	for(size_t i = 0; i < rpn_stack.size(); i++) {
		rpn_stack[i]->unref();
	}
	rpn_stack.clear();
}
void Calculator::moveRPNRegister(size_t old_index, size_t new_index) {
	if(old_index == new_index) return;
	if(old_index > 0 && old_index <= rpn_stack.size()) {
		old_index = rpn_stack.size() - old_index;
		MathStructure *mstruct = rpn_stack[old_index];
		if(new_index > rpn_stack.size()) {
			new_index = 0;
		} else if(new_index <= 1) {
			rpn_stack.push_back(mstruct);
			rpn_stack.erase(rpn_stack.begin() + old_index);
			return;
		} else {
			new_index = rpn_stack.size() - new_index;
		}
		if(new_index > old_index) {
			rpn_stack.erase(rpn_stack.begin() + old_index);
			rpn_stack.insert(rpn_stack.begin() + new_index, mstruct);
		} else if(new_index < old_index) {
			rpn_stack.insert(rpn_stack.begin() + new_index, mstruct);
			rpn_stack.erase(rpn_stack.begin() + (old_index + 1));
		}
	}
}
void Calculator::moveRPNRegisterUp(size_t index) {
	if(index > 1 && index <= rpn_stack.size()) {
		index = rpn_stack.size() - index;
		MathStructure *mstruct = rpn_stack[index];
		rpn_stack.erase(rpn_stack.begin() + index);
		index++;
		if(index == rpn_stack.size()) rpn_stack.push_back(mstruct);
		else rpn_stack.insert(rpn_stack.begin() + index, mstruct);
	}
}
void Calculator::moveRPNRegisterDown(size_t index) {
	if(index > 0 && index < rpn_stack.size()) {
		index = rpn_stack.size() - index;
		MathStructure *mstruct = rpn_stack[index];
		rpn_stack.erase(rpn_stack.begin() + index);
		index--;
		rpn_stack.insert(rpn_stack.begin() + index, mstruct);
	}
}

int has_information_unit(const MathStructure &m, bool top) {
	if(m.isUnit_exp()) {
		if(m.isUnit()) {
			if(m.unit()->baseUnit()->referenceName() == "bit") return 1;
		} else {
			if(m[0].unit()->baseUnit()->referenceName() == "bit") {
				if(m[1].isInteger() && m[1].number().isPositive()) return 1;
				return 2;
			}
		}
		return 0;
	}
	for(size_t i = 0; i < m.size(); i++) {
		int ret = has_information_unit(m[i], false);
		if(ret > 0) {
			if(ret == 1 && top && m.isMultiplication() && m[0].isNumber() && m[0].number().isFraction()) return 2;
			return ret;
		}
	}
	return 0;
}

void replace_result_cis(string &resstr) {
	gsub(" cis ", "∠", resstr);
}

// test if contains decimal value (if test_orig = true, check for decimal point in original expression)
bool contains_decimal(const MathStructure &m, const string *original_expression = NULL) {
	if(original_expression && !original_expression->empty()) return original_expression->find(DOT) != string::npos;
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_decimal(m[i])) return true;
	}
	return false;
}

// test size of simple or combined (if top value) fraction
int test_frac(const MathStructure &m, bool test_combined = true, int limit = 1000) {
	if(m.isNumber()) {
		if(!m.number().isRational()) return 0;
		if(limit < 0 || m.number().isInteger()) return 1;
		if(m.number().denominatorIsLessThan(limit)) {
			if(m.number().numeratorIsLessThan(limit) && m.number().numeratorIsGreaterThan(-limit)) return 1;
			else if(test_combined) return 2;
		}
		return 0;
	}
	if(test_combined && m.isNegate()) return test_frac(m[0], true, limit);
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_frac(m[i], false, limit)) return 0;
	}
	return 1;
}

void print_m(PrintOptions &po, const EvaluationOptions &evalops, string &str, vector<string> &results_v, MathStructure &m, const MathStructure *mresult, const string &original_expression, const MathStructure *mparse, int dfrac, int dappr, bool cplx_angle, bool only_cmp = false, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1) {
	bool b_exact = !mresult->isApproximate();
	// avoid multiple results with inequalities
	if((dfrac != 0 || dappr < 0) && (only_cmp || dfrac <= 0) && b_exact && m.isComparison() && m.comparisonType() != COMPARISON_EQUALS && po.number_fraction_format == FRACTION_DECIMAL) {
		po.number_fraction_format = (dfrac != 0 && (!m[0].isSymbolic() || !m[0].isVariable())) ? FRACTION_FRACTIONAL : FRACTION_DECIMAL_EXACT;
		m.format(po);
		str = m.print(po, format, colorize, tagtype);
		po.number_fraction_format = FRACTION_DECIMAL;
	} else {
		str = m.print(po, format, colorize, tagtype);
	}
	if(cplx_angle) replace_result_cis(str);
	// use almost equal sign in approximate equality
	if(m.isComparison() && m.comparisonType() == COMPARISON_EQUALS && po.is_approximate && *po.is_approximate) {
		size_t ipos = str.find(" = ");
		if(ipos != string::npos) {
			if(po.use_unicode_signs) {
				str.replace(ipos + 1, 1, SIGN_ALMOST_EQUAL);
			} else {
				str.insert(ipos + 3, " ");
				str.insert(ipos + 3, _("approx."));
			}
			if(po.is_approximate) *po.is_approximate = false;
		}
	}
	bool b_cmp3 = false;
	// with dual approximation, equalities contain three values
	if(m.isComparison() && m.size() == 3 && m.comparisonType() == COMPARISON_EQUALS) {
		if(m[2].isUndefined()) {
			// the only unique value is exact
			b_exact = true;
			m.delChild(3);
		} else {
			b_cmp3 = true;
		}
	}

	// do not show simple fractions in auto modes if result includes units or expression contains decimals
	if(!CALCULATOR->aborted() && (b_cmp3 || ((dfrac || (dappr && po.is_approximate && *po.is_approximate)) && (!only_cmp || (m.isComparison() && m.comparisonType() == COMPARISON_EQUALS)) && po.base == 10 && b_exact && (po.number_fraction_format == FRACTION_DECIMAL_EXACT || po.number_fraction_format == FRACTION_DECIMAL) && (dfrac > 0 || dappr > 0 || mresult->containsType(STRUCT_UNIT, false, true, true) <= 0)))) {
		bool do_frac = false, do_mixed = false;
		const MathStructure *mcmp = mresult;
		if(b_cmp3) {
			do_frac = true;
			mcmp = &m[2];
		} else if(!mparse || dfrac > 0 || ((dappr > 0 || !contains_decimal(*mparse, &original_expression)) && (!mparse->isNumber() || !mresult->isNumber()))) {
			if(contains_decimal(m)) {
				if(m.isComparison() && m.comparisonType() == COMPARISON_EQUALS) {
					mcmp = mresult->getChild(2);
				}
				int itf = test_frac(*mcmp, !only_cmp, dfrac > 0 || dappr > 0 ? -1 : 1000);
				if(itf && !mcmp->isInteger()) {
					do_frac = (itf == 1);
					if(mcmp == mresult && mparse) {
						if(mcmp->isNumber() && mcmp->number().isRational()) {
							do_frac = itf == 1 && ((evalops.parse_options.base != po.base) || ((!mparse->isDivision() || (*mparse)[1] != mcmp->number().denominator() || ((*mparse)[0] != mcmp->number().numerator() && (!(*mparse)[0].isNegate() || (*mparse)[0][0] != -mcmp->number().numerator()))) && (!mparse->isNegate() || !(*mparse)[0].isDivision() || (*mparse)[0][1] != mcmp->number().denominator() || (*mparse)[0][0] != -mcmp->number().numerator())));
							do_mixed = (dfrac != 0 || !do_frac) && !mcmp->number().isNegative() && !mcmp->number().isFraction();
						} else if(do_frac && mresult->isFunction() && mparse->isFunction() && mresult->function() == mparse->function()) {
							do_frac = false;
						}
					}
				}
			}
		}
		bool *old_approx = po.is_approximate;
		NumberFractionFormat old_fr = po.number_fraction_format;
		bool old_rfl = po.restrict_fraction_length;
		bool b_approx = false;
		po.is_approximate = &b_approx;
		if(do_frac) {
			po.number_fraction_format = FRACTION_FRACTIONAL;
			po.restrict_fraction_length = true;
			MathStructure m2(*mcmp);
			m.removeDefaultAngleUnit(evalops);
			m2.format(po);
			results_v.push_back(m2.print(po, format, colorize, tagtype));
			if(b_approx || results_v[results_v.size() - 1] == str) {
				results_v.pop_back();
			} else {
				if(cplx_angle) replace_result_cis(results_v.back());
				if(!b_cmp3 && dfrac > 0 && mparse && mparse->isNumber() && mresult->isNumber() && evalops.parse_options.base == po.base) {
					str = results_v.back();
					results_v.pop_back();
				}
			}
		}
		if(do_mixed) {
			b_approx = false;
			po.number_fraction_format = FRACTION_COMBINED;
			po.restrict_fraction_length = true;
			MathStructure m2(*mcmp);
			m.removeDefaultAngleUnit(evalops);
			m2.format(po);
			if(m2 != *mparse) {
				results_v.push_back(m2.print(po, format, colorize, tagtype));
				if(b_approx || results_v[results_v.size() - 1] == str) {
					results_v.pop_back();
				} else {
					if(cplx_angle) replace_result_cis(results_v.back());
				}
			}
		}
		// results are added to equalities instead of shown as multiple results
		if(m.isComparison() && m.comparisonType() == COMPARISON_EQUALS && !results_v.empty()) {
			size_t ipos = str.find(" = ");
			size_t ipos2 = 0;
			if(ipos == string::npos) {
				ipos = str.find(" " SIGN_ALMOST_EQUAL " ");
				if(ipos != string::npos) ipos2 = ipos + strlen(" " SIGN_ALMOST_EQUAL " ");
			} else if(!po.use_unicode_signs) {
				size_t ipos2 = str.find(_("approx."), ipos + 3);
				if(ipos2 == ipos + 3) {
					ipos2 = ipos + 4 + strlen("approx.");
				} else {
					ipos2 = ipos + 3;
				}
			} else {
				ipos2 = ipos + 3;
			}
			if(ipos != string::npos) {
				for(size_t i = results_v.size(); i > 0; i--) {
					if((max_length < 0 || (unicode_length(str) + unicode_length(results_v[i - 1]) + 3 <= (size_t) max_length)) && results_v[i - 1] != str.substr(ipos2)) {
						str.insert(ipos, results_v[i - 1]);
						str.insert(ipos, " = ");
						ipos2 += 3;
						ipos2 += results_v[i - 1].size();
					}
				}
			}
			results_v.clear();
		}
		po.number_fraction_format = old_fr;
		po.is_approximate = old_approx;
		po.restrict_fraction_length = old_rfl;
	}
}

// check for for unknown variables/symbols not on the left side of (in)equalities
bool test_fr_unknowns(const MathStructure &m) {
	if(m.isComparison()) {
		return m[1].containsUnknowns();
	} else if(m.isLogicalOr() || m.isLogicalAnd()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(test_fr_unknowns(m[i])) return true;
		}
		return false;
	}
	return m.containsUnknowns();
}
bool test_power_func(const MathStructure &m) {
	if(m.isFunction() || (m.isPower() && !m[0].containsType(STRUCT_UNIT, false, false, false) && !m[1].isInteger())) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(test_power_func(m[i])) return true;
	}
	return false;
}
bool test_parsed_comparison(const MathStructure &m) {
	if(m.isComparison()) return true;
	if((m.isLogicalOr() || m.isLogicalAnd()) && m.size() > 0) {
		for(size_t i = 0; i < m.size(); i++) {
			if(!test_parsed_comparison(m[i])) return false;
		}
		return true;
	}
	return false;
}

bool shown_with_scientific_notation(const Number &nr, const PrintOptions &po) {
	if(po.min_exp == EXP_NONE || po.base != 10) return false;
	CALCULATOR->beginTemporaryStopMessages();
	string exp;
	InternalPrintStruct ips;
	PrintOptions po2 = po;
	po2.is_approximate = NULL;
	ips.exp = &exp;
	nr.print(po2, ips);
	CALCULATOR->endTemporaryStopMessages();
	return !exp.empty();
}

void print_dual(const MathStructure &mresult, const string &original_expression, const MathStructure &mparse, MathStructure &mexact, string &result_str, vector<string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle, bool *exact_cmp, bool b_parsed, bool format, int colorize, int tagtype, int max_length, bool converted) {

	MathStructure m(mresult);

	if(po.spell_out_logical_operators && test_parsed_comparison(mparse)) {
		if(m.isZero()) {
			Variable *v = CALCULATOR->getActiveVariable("false");
			if(v) m.set(v);
		} else if(m.isOne()) {
			Variable *v = CALCULATOR->getActiveVariable("true");
			if(v) m.set(v);
		}
	}

	// convert time units to hours when using time format
	if(po.base == BASE_TIME) {
		bool b = false;
		if(m.isUnit() && m.unit()->baseUnit()->referenceName() == "s") {
			b = true;
		} else if(m.isMultiplication() && m.size() == 2 && m[0].isNumber() && m[1].isUnit() && m[1].unit()->baseUnit()->referenceName() == "s") {
			b = true;
		} else if(m.isAddition() && m.size() > 0) {
			b = true;
			for(size_t i = 0; i < m.size(); i++) {
				if(m[i].isUnit() && m[i].unit()->baseUnit()->referenceName() == "s") {}
				else if(m[i].isMultiplication() && m[i].size() == 2 && m[i][0].isNumber() && m[i][1].isUnit() && m[i][1].unit()->baseUnit()->referenceName() == "s") {}
				else {b = false; break;}
			}
		}
		if(b) {
			Unit *u = CALCULATOR->getActiveUnit("h");
			if(u) {
				m.divide(u);
				m.eval(evalops);
			}
		}
	}

	// dual and auto approximation and fractions
	results_v.clear();
	if(exact_cmp) *exact_cmp = false;
	bool *save_is_approximate = po.is_approximate;
	bool tmp_approx = false;
	if(!po.is_approximate) po.is_approximate = &tmp_approx;

	// dual/auto approximation: requires that the value is approximate and exact result is set
	bool do_exact = !mexact.isUndefined() && m.isApproximate();

	// If parsed value is number (simple fractions are parsed as division) only show result as combined fraction
	if((auto_frac == AUTOMATIC_FRACTION_AUTO || auto_frac == AUTOMATIC_FRACTION_SINGLE) && po.base == 10 && po.base == evalops.parse_options.base && !m.isApproximate() && b_parsed && mparse.isNumber() && !mparse.isInteger() && !converted && !shown_with_scientific_notation(mparse.number(), po)) {
		po.number_fraction_format = FRACTION_COMBINED;
	// with auto fractions show expressions with unknown variables/symbols only using simple fractions (if not parsed value contains decimals)
	} else if((auto_frac == AUTOMATIC_FRACTION_AUTO || auto_frac == AUTOMATIC_FRACTION_SINGLE) && !m.isApproximate() && (test_fr_unknowns(m) || (m.containsType(STRUCT_ADDITION) && test_power_func(m))) && test_frac(m, false, -1) && (!b_parsed || !contains_decimal(mparse, &original_expression))) {
		po.number_fraction_format = FRACTION_FRACTIONAL;
		po.restrict_fraction_length = true;
	// with auto fractions and exact approximation (or inequality) do not show approximate decimal fractions
	} else if(auto_frac != AUTOMATIC_FRACTION_OFF && po.number_fraction_format == FRACTION_DECIMAL && (evalops.approximation == APPROXIMATION_EXACT || (auto_frac != AUTOMATIC_FRACTION_DUAL && m.isComparison() && m.comparisonType() != COMPARISON_EQUALS))) {
		po.number_fraction_format = FRACTION_DECIMAL_EXACT;
	}

	// for equalities, append exact values to approximate value (as third part of equalities)
	if(do_exact) {
		if(mexact.isComparison() && mexact.comparisonType() == COMPARISON_EQUALS) {
			mexact[1].ref();
			if(m.isComparison()) {
				m.addChild_nocopy(&mexact[1]);
				if(exact_cmp) *exact_cmp = true;
			} else {
				// only the first exact value is used from logical and
				m[0].addChild_nocopy(&mexact[1]);
			}
		} else if(mexact.isLogicalOr()) {
			if(exact_cmp) *exact_cmp = true;
			for(size_t i = 0; i < mexact.size(); i++) {
				mexact[i][1].ref();
				if(m[i].isComparison()) {
					m[i].addChild_nocopy(&mexact[i][1]);
				} else {
					// only the first exact value is used from logical and
					m[i][0].addChild_nocopy(&mexact[i][1]);
					if(exact_cmp) *exact_cmp = false;
				}
			}
		}
	}

	m.removeDefaultAngleUnit(evalops);
	m.format(po);
	m.removeDefaultAngleUnit(evalops);

	if(auto_frac != AUTOMATIC_FRACTION_OFF && po.base == 10 && po.base == evalops.parse_options.base && po.number_fraction_format == FRACTION_DECIMAL_EXACT && !m.isApproximate() && mresult.isNumber() && !mresult.number().isFraction() && !mresult.number().isNegative() && b_parsed && mparse.equals(m)) {
		po.number_fraction_format = FRACTION_COMBINED;
		m = mresult;
		m.removeDefaultAngleUnit(evalops);
		m.format(po);
		m.removeDefaultAngleUnit(evalops);
	}
	int dappr = 0, dfrac = 0;
	if(auto_frac == AUTOMATIC_FRACTION_AUTO) dfrac = -1;
	else if(auto_frac == AUTOMATIC_FRACTION_DUAL) dfrac = 1;
	if(auto_approx == AUTOMATIC_APPROXIMATION_AUTO) dappr = -1;
	else if(auto_approx == AUTOMATIC_APPROXIMATION_DUAL) dappr = 1;
	if(m.isLogicalOr()) {
		if(max_length > 0) max_length /= m.size();
		InternalPrintStruct ips;
		bool b_approx = false;
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) {result_str = CALCULATOR->abortedMessage(); break;}
			if(i == 0) {
				result_str = "";
			} else if(po.spell_out_logical_operators) {
				result_str += " ";
				result_str += _("or");
				result_str += " ";
			} else {
				if(po.spacious) result_str += " ";
				result_str += "||";
				if(po.spacious) result_str += " ";
			}
			if(m[i].isLogicalAnd() && (po.preserve_format || m[i].size() != 2 || !m[i][0].isComparison() || !m[i][1].isComparison() || m[i][0].comparisonType() == COMPARISON_EQUALS || m[i][0].comparisonType() == COMPARISON_NOT_EQUALS || m[i][1].comparisonType() == COMPARISON_EQUALS || m[i][1].comparisonType() == COMPARISON_NOT_EQUALS || m[i][0][0] != m[i][1][0])) {
				int ml = max_length / m[i].size();
				for(size_t i2 = 0; i2 < m[i].size(); i2++) {
					if(CALCULATOR->aborted()) {result_str = CALCULATOR->abortedMessage(); break;}
					if(i2 > 0) {
						if(po.spell_out_logical_operators) {
							result_str += " ";
							result_str += _("and");
							result_str += " ";
						} else {
							if(po.spacious) result_str += " ";
							result_str += "&&";
							if(po.spacious) result_str += " ";
						}
					}
					bool b_wrap = m[i][i2].needsParenthesis(po, ips, m[i], i2 + 1, true, true);
					string str;
					print_m(po, evalops, str, results_v, m[i][i2], &m[i][i2], original_expression, &mparse, dfrac, dappr, cplx_angle, true, format, colorize, tagtype, ml);
					if(b_wrap) result_str += "(";
					result_str += str;
					if(b_wrap) result_str += ")";
					if(b_approx) *po.is_approximate = true;
					b_approx = *po.is_approximate;
				}
			} else {
				bool b_wrap = m[i].needsParenthesis(po, ips, m, i + 1, true, true);
				string str;
				print_m(po, evalops, str, results_v, m[i], &m[i], original_expression, &mparse, dfrac, dappr, cplx_angle, true, format, colorize, tagtype, max_length);
				if(b_wrap) result_str += "(";
				result_str += str;
				if(b_wrap) result_str += ")";
				if(b_approx) *po.is_approximate = true;
				b_approx = *po.is_approximate;
			}
		}
	} else if(m.isLogicalAnd() && (po.preserve_format || m.size() != 2 || !m[0].isComparison() || !m[1].isComparison() || m[0].comparisonType() == COMPARISON_EQUALS || m[0].comparisonType() == COMPARISON_NOT_EQUALS || m[1].comparisonType() == COMPARISON_EQUALS || m[1].comparisonType() == COMPARISON_NOT_EQUALS || m[0][0] != m[1][0])) {
		InternalPrintStruct ips;
		bool b_approx = false;
		max_length /= m.size();
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) {result_str = CALCULATOR->abortedMessage(); break;}
			if(i == 0) {
				result_str = "";
			} else if(po.spell_out_logical_operators) {
				result_str += " ";
				result_str += _("and");
				result_str += " ";
			} else {
				if(po.spacious) result_str += " ";
				result_str += "&&";
				if(po.spacious) result_str += " ";
			}
			bool b_wrap = m[i].needsParenthesis(po, ips, m, i + 1, true, true);
			string str;
			print_m(po, evalops, str, results_v, m[i], &m[i], original_expression, &mparse, dfrac, dappr, cplx_angle, true, format, colorize, tagtype, max_length);
			if(b_wrap) result_str += "(";
			result_str += str;
			if(b_wrap) result_str += ")";
			if(b_approx) *po.is_approximate = true;
			b_approx = *po.is_approximate;
		}
	} else {
		print_m(po, evalops, result_str, results_v, m, &mresult, original_expression, &mparse, dfrac, dappr, cplx_angle, false, format, colorize, tagtype, max_length);
		if(do_exact && (m.type() != STRUCT_COMPARISON || m.comparisonType() != COMPARISON_EQUALS)) {
			bool *old_approx = po.is_approximate;
			bool b_approx = false;
			po.is_approximate = &b_approx;
			MathStructure mexact2(mexact);
			po.number_fraction_format = FRACTION_FRACTIONAL;
			po.restrict_fraction_length = true;
			mexact2.removeDefaultAngleUnit(evalops);
			mexact2.format(po);
			mexact2.removeDefaultAngleUnit(evalops);
			string str = mexact2.print(po, format, colorize, tagtype);
			if(cplx_angle) replace_result_cis(str);
			if(!b_approx) {
				results_v.push_back(str);
			}
			po.is_approximate = old_approx;
		}
	}
	po.is_approximate = save_is_approximate;
	if(po.is_approximate && result_str == _("aborted")) {
		*po.is_approximate = false;
	}
}

bool test_simplified2(const MathStructure &m1, const MathStructure &m2) {
	if(m1.type() != m2.type() || m1.size() != m2.size()) return false;
	if(m1.isNumber()) {
		return comparison_might_be_equal(m1.number().compare(m2.number()));
	}
	if(m1.size() == 0) return m1.equals(m2, true, true);
	for(size_t i = 0; i < m1.size(); i++) {
		if(!test_simplified2(m1[i], m2[i])) return false;
	}
	return true;
}
bool test_simplified(const MathStructure &m, bool top = true) {
	if(m.isFunction() || (m.isVariable() && m.variable()->isKnown()) || (m.isUnit() && (m.unit()->hasApproximateRelationToBase() || (m.unit()->isCurrency() && m.unit() != CALCULATOR->getLocalCurrency())))) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_simplified(m[i], false)) return false;
	}
	if(m.isPower() && m[0].containsType(STRUCT_NUMBER)) return false;
	if(!top && m.isNumber() && m.number().isFloatingPoint()) return false;
	return true;
}
void flatten_addmulti(MathStructure &m) {
	if(m.isMultiplication() || m.isAddition()) {
		for(size_t i = 0; i < m.size();) {
			if(m[i].type() == m.type()) {
				for(size_t i2 = 0; i2 < m[i].size(); i2++) {
					m[i][i2].ref();
					m.insertChild_nocopy(&m[i][i2], i + i2 + 2);
				}
				m.delChild(i + 1);
			} else {
				i++;
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		flatten_addmulti(m[i]);
	}
}
void replace_negdiv(MathStructure &m) {
	if(m.isDivision() && m[1].isMultiplication() && m[1].size() > 0) {
		for(size_t i = 0; i < m[1].size(); i++) {
			m[1][i].transform(STRUCT_INVERSE);
		}
		if(m[0].isOne()) {
			m.setToChild(2);
		} else {
			m[0].ref();
			MathStructure *m0 = &m[0];
			m.setToChild(2);
			m.insertChild_nocopy(m0, 1);
		}
		return replace_negdiv(m);
	}
	if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isMinusOne()) {
				for(size_t i2 = 0; i2 < m.size(); i2++) {
					if(i2 != i && m[i2].isNumber()) {
						m[i2].number().negate();
						m.delChild(i + 1, true);
						return replace_negdiv(m);
					}
				}
			}
		}
	}
	if(m.isDivision() && m[1].isPower()) {
		if(m[1][1].isNegate()) {
			m[1][1].setToChild(1);
		} else {
			m[1][1].transform(STRUCT_NEGATE);
		}
		if(m[0].isOne()) {
			m.setToChild(2);
		} else {
			m.setType(STRUCT_MULTIPLICATION);
		}
		return replace_negdiv(m);
	}
	if(m.isInverse() && m[0].isMultiplication() && m[0].size() > 0) {
		for(size_t i = 0; i < m[0].size(); i++) {
			m[0][i].transform(STRUCT_INVERSE);
		}
		m.setToChild(1);
		return replace_negdiv(m);
	}
	if(m.isInverse() && m[0].isPower()) {
		if(m[0][1].isNegate()) {
			m[0][1].setToChild(1);
		} else {
			m[0][1].transform(STRUCT_NEGATE);
		}
		m.setToChild(1);
		return replace_negdiv(m);
	}
	if(m.isNegate()) {
		if(m[0].isNumber()) {
			m[0].number().negate();
			m.setToChild(1);
			return replace_negdiv(m);
		} else if((m[0].isDivision() || m[0].isInverse() || m[0].isMultiplication()) && m[0].size() > 0) {
			m[0][0].transform(STRUCT_NEGATE);
			m.setToChild(1);
			return replace_negdiv(m);
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_negdiv(m[i]);
	}
	if(m.isFunction()) {
		if(m.function()->id() == FUNCTION_ID_SQRT && m.size() == 1) {
			m.setType(STRUCT_POWER);
			m.addChild(nr_half);
		} else if(m.function()->id() == FUNCTION_ID_CBRT && m.size() == 1) {
			if(m[0].representsNonNegative()) {
				m.setType(STRUCT_POWER);
				m.addChild(Number(1, 3));
			} else if(m[0].representsNonPositive()) {
				if(m[0].isNumber()) m[0].number().negate();
				else m[0].negate();
				m.setType(STRUCT_POWER);
				m.addChild(Number(1, 3));
				m.negate();
			} else {
				m.setFunctionId(FUNCTION_ID_ROOT);
				m.addChild(nr_three);
			}
		} else if(m.function()->id() == FUNCTION_ID_ROOT && m.size() == 2 && m[1].isInteger()) {
			if(m[0].representsNonNegative()) {
				m.setType(STRUCT_POWER);
				m[1].number().recip();
			} else if(m[1].representsOdd() && m[0].representsNonPositive()) {
				if(m[0].isNumber()) m[0].number().negate();
				else m[0].negate();
				m[1].number().recip();
				m.setType(STRUCT_POWER);
				m.negate();
			}
		}
	}
	if(m.isDivision()) {
		if(m[1].isInteger()) {
			if(m[0].isMultiplication() && m[0].size() > 0) {
				m[0].unformat();
				flatten_addmulti(m[0]);
				m[0].evalSort(false);
				if(m[0][0].isInteger()) {
					m[0][0].number() /= m[1].number();
					m.setToChild(1);
				} else {
					m.setType(STRUCT_MULTIPLICATION);
					m[1].number().recip();
				}
			} else if(m[0].isInteger()) {
				m[0].number() /= m[1].number();
				m.setToChild(1);
			} else {
				m.setType(STRUCT_MULTIPLICATION);
				m[1].number().recip();
			}
		}
	}
}
void replace_zero_symbol(MathStructure &m) {
	if(m.isFunction()) {
		for(size_t i = 1; i < m.size(); i++) {
			Argument *arg = m.function()->getArgumentDefinition(i + 1);
			if(arg && arg->type() == ARGUMENT_TYPE_SYMBOLIC && (m[i].isZero() || m[i].isUndefined())) {
				m[i].set(m[0].find_x_var(), true);
				if(m[i].isUndefined() && m[0].isVariable() && m[0].variable()->isKnown()) m[i].set(((KnownVariable*) m[0].variable())->get().find_x_var(), true);
				if(m[i].isUndefined()) m[i].set(CALCULATOR->getVariableById(VARIABLE_ID_X), true);
			}
		}
	} else if(m.isVariable() && m.variable() == CALCULATOR->getVariableById(VARIABLE_ID_UNDEFINED)) {
		m.setUndefined();
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_zero_symbol(m[i]);
	}
}
bool comparison_compare(const MathStructure &m1, MathStructure &m2) {
	if(m1.isNumber() && m2.isNumber()) {
		ComparisonResult cr = m1.number().compare(m2.number(), true);
		if(cr == COMPARISON_RESULT_EQUAL || cr > COMPARISON_RESULT_UNKNOWN) {
			if(m1.number().hasImaginaryPart() || m2.number().hasImaginaryPart()) {
				cr = m1.number().compareImaginaryParts(m2.number());
				return cr == COMPARISON_RESULT_EQUAL || cr > COMPARISON_RESULT_UNKNOWN;
			}
			return true;
		}
		return false;
	} else {
		if(m1.type() == m2.type() && m1.size() == m2.size()) {
			if(m1.size() == 0) return m1.equals(m2, true);
			if(m1.type() == STRUCT_COMPARISON) {
				if(m1.comparisonType() != m2.comparisonType()) return false;
			} else if(m1.type() == STRUCT_FUNCTION) {
				if(m1.function() != m2.function()) return false;
			}
			for(size_t i = 0; i < m1.size(); i++) {
				if(!comparison_compare(m1[i], m2[i])) return false;
			}
			return true;
		} else if(m1.isAddition() && m2.isAddition()) {
			if(m1.size() + 1 == m2.size() && m2.last().isNumber() && !m2.last().number().isNonZero()) {
				for(size_t i = 0; i < m1.size(); i++) {
					if(!comparison_compare(m1[i], m2[i])) return false;
				}
				m2.delChild(m2.size(), true);
				return true;
			} else if(m2.size() + 1 == m1.size() && m1.last().isNumber() && !m1.last().number().isNonZero()) {
				for(size_t i = 0; i < m2.size(); i++) {
					if(!comparison_compare(m1[i], m2[i])) return false;
				}
				return true;
			}
		} else if(m1.isMultiplication() && m2.isMultiplication()) {
			if(m1.size() + 1 == m2.size() && m2[0].isNumber() && !m2[0].number().imaginaryPartIsNonZero()) {
				for(size_t i = 0; i < m1.size(); i++) {
					if(!comparison_compare(m1[i], m2[i + 1])) return false;
				}
				ComparisonResult cr = m2[0].number().compare(nr_one, true);
				if(cr != COMPARISON_RESULT_EQUAL && cr <= COMPARISON_RESULT_UNKNOWN) return false;
				m2.delChild(1, true);
				return true;
			} else if(m2.size() + 1 == m1.size() && m1[0].isNumber() && !m1[0].number().imaginaryPartIsNonZero()) {
				for(size_t i = 0; i < m2.size(); i++) {
					if(!comparison_compare(m1[i + 1], m2[i])) return false;
				}
				ComparisonResult cr = m1[0].number().compare(nr_one, true);
				if(cr != COMPARISON_RESULT_EQUAL && cr <= COMPARISON_RESULT_UNKNOWN) return false;
			}
		} else if(m2.isAddition() && m2.size() == 2 && m2.last().isNumber() && !m2.last().number().isNonZero()) {
			if(comparison_compare(m1, m2[0])) {
				m2.delChild(m2.size(), true);
				return true;
			}
		} else if(m1.isAddition() && m1.size() == 2 && m1.last().isNumber() && !m1.last().number().isNonZero()) {
			if(comparison_compare(m1[0], m2)) {
				return true;
			}
		} else if(m2.isMultiplication() && m2.size() == 2 && m2[0].isNumber() && !m2[0].number().imaginaryPartIsNonZero()) {
			if(!comparison_compare(m1, m2[1])) return false;
			ComparisonResult cr = m2[0].number().compare(nr_one, true);
			if(cr != COMPARISON_RESULT_EQUAL && cr <= COMPARISON_RESULT_UNKNOWN) return false;
			m2.delChild(1, true);
			return true;
		} else if(m1.isMultiplication() && m1.size() == 2 && m1[0].isNumber() && !m1[0].number().imaginaryPartIsNonZero()) {
			if(!comparison_compare(m1[1], m2)) return false;
			ComparisonResult cr = m1[0].number().compare(nr_one, true);
			if(cr != COMPARISON_RESULT_EQUAL && cr <= COMPARISON_RESULT_UNKNOWN) return false;
			return true;
		}
	}
	return false;
}
bool test_max_addition_size(const MathStructure &m, size_t n) {
	if(m.isAddition() && m.size() > n) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(test_max_addition_size(m[i], n)) return true;
	}
	return false;
}

bool equals_with_vname(const MathStructure &m1, const MathStructure &m2) {
	if(m1.size() != m2.size() || m1.type() != m2.type()) return false;
	if(m1.isVariable() && m2.isVariable()) {
		if(m1.variable() == m2.variable() || m1.variable()->name() == m2.variable()->name()) return true;
		return false;
	}
	if(m1.size() == 0) return m1.equals(m2, true, true);
	if(m1.isComparison() && m1.comparisonType() != m2.comparisonType()) return false;
	if(m1.isFunction() && m1.function() != m2.function()) return false;
	for(size_t i = 0; i < m1.size(); i++) {
		if(!equals_with_vname(m1[i], m2[i])) return false;
	}
	return true;
}

void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs, int max_size) {
	int dual_approximation = 0;
	if(auto_approx == AUTOMATIC_APPROXIMATION_AUTO || auto_approx == AUTOMATIC_APPROXIMATION_SINGLE) dual_approximation = -1;
	else if(auto_approx == AUTOMATIC_APPROXIMATION_DUAL) dual_approximation = 1;
	if(dual_approximation != 0 && evalops.approximation == APPROXIMATION_TRY_EXACT && mstruct->isApproximate() && (dual_approximation > 0 || (!mstruct->containsType(STRUCT_UNIT, false, false, false) && !parsed_mstruct->containsType(STRUCT_UNIT, false, false, false) && original_expression.find(DOT) == string::npos)) && !parsed_mstruct->containsFunctionId(FUNCTION_ID_SAVE) && !parsed_mstruct->containsFunctionId(FUNCTION_ID_PLOT) && !parsed_mstruct->containsFunctionId(FUNCTION_ID_RAND) && !parsed_mstruct->containsFunctionId(FUNCTION_ID_RANDN) && !parsed_mstruct->containsFunctionId(FUNCTION_ID_RAND_POISSON) && !parsed_mstruct->containsInterval(true, false, false, false, true)) {
		ApproximationMode approx_bak = evalops.approximation;
		int expand_bak = evalops.expand;
		evalops.approximation = APPROXIMATION_EXACT;
		evalops.expand = -2;
		CALCULATOR->beginTemporaryStopMessages();
		if(msecs > 0) CALCULATOR->startControl(msecs);
		MathStructure tmp_parse;
		mstruct_exact = CALCULATOR->calculate(original_expression, evalops, &tmp_parse);
		evalops.approximation = approx_bak;
		evalops.expand = expand_bak;
		if(CALCULATOR->aborted() || mstruct_exact.isApproximate() || (parsed_mstruct && !equals_with_vname(tmp_parse, *parsed_mstruct)) || (dual_approximation < 0 && max_size > 0 && (test_max_addition_size(mstruct_exact, (size_t) max_size) || mstruct_exact.countTotalChildren(false) > (size_t) max_size * 6))) {
			mstruct_exact.setUndefined();
		} else if(test_simplified(mstruct_exact) || test_simplified2(mstruct_exact, *mstruct)) {
			mstruct->set(mstruct_exact);
			mstruct_exact.setUndefined();
		} else if(auto_approx == AUTOMATIC_APPROXIMATION_SINGLE) {
			mstruct_exact.setUndefined();
		}
		if(mstruct_exact.containsType(STRUCT_COMPARISON)) {
			bool b = false;
			MathStructure mbak(*mstruct);
			MathStructure *mcmp = mstruct;
			if(mcmp->isLogicalAnd() && mcmp->size() > 0) mcmp = &(*mcmp)[0];
			if(mcmp->isComparison() && (mcmp->comparisonType() == COMPARISON_EQUALS || dual_approximation > 0) && !(*mcmp)[0].isNumber()) {
				if(mstruct_exact.isLogicalAnd() && mstruct_exact.size() > 0) mstruct_exact.setToChild(1);
				if(mstruct_exact.isComparison() && mstruct_exact.comparisonType() == mcmp->comparisonType() && mstruct_exact[0] == (*mcmp)[0]) {
					MathStructure mtest(mstruct_exact[1]);
					mtest.eval(evalops);
					MathStructure m2((*mcmp)[1]);
					if(comparison_compare(mtest, m2)) {
						if(!mtest.isApproximate()) {(*mcmp)[1].set(mtest); mstruct_exact[1].setUndefined();}
						else (*mcmp)[1].set(m2);
						b = true;
					}
				} else if(mstruct_exact.isLogicalOr()) {
					for(size_t i = 0; i < mstruct_exact.size(); i++) {
						if(mstruct_exact[i].isLogicalAnd() && mstruct_exact[i].size() > 0) mstruct_exact[i].setToChild(1);
						if(mstruct_exact[i].isComparison() && mstruct_exact[i].comparisonType() == mcmp->comparisonType() && mstruct_exact[i][0] == (*mcmp)[0]) {
							MathStructure mtest(mstruct_exact[i][1]);
							if(CALCULATOR->aborted()) break;
							mtest.eval(evalops);
							MathStructure m2((*mcmp)[1]);
							if(comparison_compare(mtest, m2)) {
								mstruct_exact.setToChild(i + 1);
								if(!mtest.isApproximate()) {(*mcmp)[1].set(mtest); mstruct_exact[1].setUndefined();}
								else (*mcmp)[1].set(m2);
								b = true;
								break;
							}
						}
					}
				}
			} else if(mcmp->isLogicalOr()) {
				if(mstruct_exact.isLogicalOr() && mstruct_exact.size() >= mcmp->size()) {
					b = true;
					for(size_t i = 0; i < mcmp->size() && b; i++) {
						MathStructure *mcmpi = &(*mcmp)[i];
						if(mcmpi->isLogicalAnd() && mcmpi->size() > 0) mcmpi = &(*mcmpi)[0];
						if(!mcmpi->isComparison() || mcmpi->comparisonType() != COMPARISON_EQUALS || (*mcmpi)[0].isNumber()) {b = false; break;}
						for(size_t i2 = 0; i2 < mstruct_exact.size();) {
							if(mstruct_exact[i2].isProtected()) {
								i2++;
							} else {
								if(mstruct_exact[i2].isLogicalAnd() && mstruct_exact[i2].size() > 0) mstruct_exact[i2].setToChild(1);
								if(mstruct_exact[i2].isComparison() && mstruct_exact[i2].comparisonType() == mcmpi->comparisonType() && mstruct_exact[i2][0] == (*mcmpi)[0]) {
									MathStructure mtest(mstruct_exact[i2][1]);
									if(CALCULATOR->aborted()) break;
									mtest.eval(evalops);
									MathStructure m2((*mcmpi)[1]);
									if(comparison_compare(mtest, m2)) {
										if(!mtest.isApproximate()) {(*mcmpi)[1].set(mtest); mstruct_exact[i2][1].setUndefined();}
										else (*mcmpi)[1].set(m2);
										mstruct_exact[i2].setProtected();
										break;
									}
								}
								if(mstruct_exact.size() <= mcmp->size()) {b = false; break;}
								mstruct_exact.delChild(i2 + 1);
							}
						}
					}
					if(b) {
						for(size_t i2 = 0; i2 < mstruct_exact.size(); i2++) mstruct_exact[i2].setProtected(false);
					}
				}
			}
			if(!b) mstruct->set(mbak);
			if(!b) mstruct_exact.setUndefined();
		}
		if(!mstruct_exact.isUndefined()) {
			MathStructure mtest(*parsed_mstruct);
			flatten_addmulti(mtest);
			replace_negdiv(mtest);
			mtest.unformat(evalops);
			flatten_addmulti(mtest);
			replace_zero_symbol(mtest);
			mtest.evalSort(true);
			if(mstruct_exact.equals(mtest, true, true)) mstruct_exact.setUndefined();
		}
		if(msecs > 0) CALCULATOR->stopControl();
		CALCULATOR->endTemporaryStopMessages();
	}
}

bool expression_contains_save_function(const string &str, const ParseOptions &po, bool only_equals) {
	if(str.length() < 2 || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && CALCULATOR->customInputBase() > 62)) return false;
	size_t i = str.find("=", 1);
	if(!only_equals) {
		if(i != string::npos && ((i > 0 && str[i - 1] == ':') || (i < str.length() - 1 && str[i + 1] == ':'))) return true;
		MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_SAVE);
		for(size_t i2 = 1; f && i2 <= f->countNames(); i2++) {
			if(str.find(f->getName(i2).name) != string::npos) return true;
		}
	}
	if(i == string::npos) return false;
	if(i < str.length() - 1) {
		size_t i3 = str.find_first_not_of(SPACES, i + 1);
		if(i3 != string::npos && (str[i3] == ':' || str[i3] == '!' || str[i3] == '<' || str[i3] == '>')) return false;
	}
	size_t i_name1 = str.find_first_not_of(SPACES);
	if(i_name1 == i) return false;
	size_t i_name2 = str.find_last_not_of(SPACES, i - 1);
	if(i_name2 == string::npos) return false;
	bool b_quote = i_name2 - i_name1 >= 2 && ((str[i_name2] == '\'' || str[i_name2] == '\"') && str[i_name1] == str[i_name2] && str.rfind(str[i_name1], i_name2 - 1) == i_name1);
	if(!b_quote) b_quote = (i_name2 - i_name1 >= 3 && ((str[i_name1] == '\xe2' && str[i_name1 + 1] == '\x80') || (str[i_name1] == '\xc2' && (str[i_name1 + 1] == '\xab' || str[i_name1 + 1] == '\xbb'))));
	for(size_t i2 = i_name1; b_quote && i2 < i_name2; i2++) {
		if(str[i2] == '\xe2' || str[i2] == '\xc2') {
			string name = str.substr(i_name1, i_name2 - i_name1 + 1);
			CALCULATOR->parseSigns(name);
			b_quote = (name.length() >= 2 && (name[0] == '\'' || name[0] == '\"') && name[0] == name[name.length() - 1] && name.rfind(name[0], name.length() - 2) == 0);
			break;
		}
	}
	if(!b_quote && (str[i_name2] == ':' || str[i_name2] == '!' || str[i_name2] == '<' || str[i_name2] == '>')) return false;
	bool b_func = false;
	if(!b_quote && i_name2 - i_name1 >= 2 && str[i_name2] == RIGHT_PARENTHESIS_CH) {
		i_name2 = str.find_last_not_of(SPACES, i_name2 - 1);
		if(i_name2 == string::npos || i_name2 == 0 || str[i_name2] != LEFT_PARENTHESIS_CH) return false;
		i_name2 = str.find_last_not_of(SPACES, i_name2 - 1);
		if(i_name2 == string::npos) return false;
		b_func = true;
	}
	if(!b_quote && !CALCULATOR->variableNameIsValid(str.substr(i_name1, i_name2 - i_name1 + 1))) return false;
	string name = str.substr(i_name1, i_name2 - i_name1 + 1);
	if(!b_quote) {
		if(name.find("#") != string::npos) return false;
		CALCULATOR->parseSigns(name);
		if(!CALCULATOR->variableNameIsValid(name)) return false;
		name = str.substr(i_name1, i_name2 - i_name1 + 1);
		if(!b_quote && name.length() > 1 && name[name.length() - 1] == 'x') {
			MathFunction *f = CALCULATOR->getActiveFunction(name.substr(0, name.length() - 1));
			if(f && f->minargs() <= 1 && f->maxargs() != 0) return false;
			if(!f && CALCULATOR->getActiveVariable(name.substr(0, name.length() - 1))) return false;
		}
	}
	EvaluationOptions eo;
	eo.parse_options = po;
	if(CALCULATOR->hasWhereExpression(str, eo) && str.rfind(_("where"), i - 1) == string::npos && str.rfind("where", i - 1) == string::npos && str.rfind("/.", str.length() - 2) == string::npos) {
		return false;
	}
	size_t i2 = str.find(name, i);
	if(i2 != string::npos && str.rfind("#", i2 - 1) == string::npos) {
		if(b_quote) return false;
		string value = str.substr(i + 1, str.length() - (i + 1));
		CALCULATOR->parseComments(value);
		string stmp;
		CALCULATOR->separateToExpression(value, stmp, eo);
		CALCULATOR->separateWhereExpression(value, stmp, eo);
		CALCULATOR->parseSigns(value);
		size_t i2 = str.find(name, i);
		if(i2 != string::npos) {
			ExpressionItem *item1 = CALCULATOR->getActiveExpressionItem(name);
			ExpressionItem *item2 = item1 ? CALCULATOR->getActiveExpressionItem(name, item1) : NULL;
			if(item1) {
				MathStructure mtest;
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mtest, str.substr(i + 1, str.length() - (i + 1)), po);
				CALCULATOR->endTemporaryStopMessages();
				if(!b_func && item1->type() == TYPE_VARIABLE && !((Variable*) item1)->isKnown() && mtest.contains((Variable*) item1, true, true, false)) return false;
				else if(!b_func && item1->type() == TYPE_UNIT && mtest.contains((Unit*) item1, true, true, false)) return false;
				else if(b_func && item1->type() == TYPE_FUNCTION && mtest.containsFunction((MathFunction*) item1, true, true, false)) return false;
				if(!b_func && item2 && item2->type() == TYPE_VARIABLE && !((Variable*) item2)->isKnown() && mtest.contains((Variable*) item2, true, true, false)) return false;
				else if(!b_func && item2 && item2->type() == TYPE_UNIT && mtest.contains((Unit*) item2, true, true, false)) return false;
				else if(b_func && item2 && item2->type() == TYPE_FUNCTION && mtest.containsFunction((MathFunction*) item2, true, true, false)) return false;
			}
		}
	}
	return true;
}
bool transform_expression_for_equals_save(string &str, const ParseOptions &po) {
	if(str.length() < 2 || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && CALCULATOR->customInputBase() > 62)) return false;
	size_t i = str.find("=", 1);
	if(i == string::npos) return false;
	if(i < str.length() - 1) {
		size_t i3 = str.find_first_not_of(SPACES, i + 1);
		if(i3 != string::npos && (str[i3] == ':' || str[i3] == '!' || str[i3] == '<' || str[i3] == '>')) return false;
	}
	size_t i_name1 = str.find_first_not_of(SPACES);
	if(i_name1 == i) return false;
	size_t i_name2 = str.find_last_not_of(SPACES, i - 1);
	if(i_name2 == string::npos) return false;
	bool b_quote = i_name2 - i_name1 >= 2 && ((str[i_name2] == '\'' || str[i_name2] == '\"') && str[i_name1] == str[i_name2] && str.rfind(str[i_name1], i_name2 - 1) == i_name1);
	if(!b_quote) b_quote = (i_name2 - i_name1 >= 3 && ((str[i_name1] == '\xe2' && str[i_name1 + 1] == '\x80') || (str[i_name1] == '\xc2' && (str[i_name1 + 1] == '\xab' || str[i_name1 + 1] == '\xbb'))));
	for(size_t i2 = i_name1; b_quote && i2 < i_name2; i2++) {
		if(str[i2] == '\xe2' || str[i2] == '\xc2') {
			string name = str.substr(i_name1, i_name2 - i_name1 + 1);
			CALCULATOR->parseSigns(name);
			b_quote = (name.length() >= 2 && (name[0] == '\'' || name[0] == '\"') && name[0] == name[name.length() - 1] && name.rfind(name[0], name.length() - 2) == 0);
			break;
		}
	}
	if(!b_quote && (str[i_name2] == ':' || str[i_name2] == '!' || str[i_name2] == '<' || str[i_name2] == '>')) return false;
	bool b_func = false;
	if(!b_quote && i_name2 - i_name1 >= 2 && str[i_name2] == RIGHT_PARENTHESIS_CH) {
		i_name2 = str.find_last_not_of(SPACES, i_name2 - 1);
		if(i_name2 == string::npos || i_name2 == 0 || str[i_name2] != LEFT_PARENTHESIS_CH) return false;
		i_name2 = str.find_last_not_of(SPACES, i_name2 - 1);
		if(i_name2 == string::npos) return false;
		b_func = true;
	}
	if(!b_quote && !CALCULATOR->variableNameIsValid(str.substr(i_name1, i_name2 - i_name1 + 1))) return false;
	string name = str.substr(i_name1, i_name2 - i_name1 + 1);
	if(!b_quote) {
		if(name.find("#") != string::npos) return false;
		CALCULATOR->parseSigns(name);
		if(!CALCULATOR->variableNameIsValid(name)) return false;
		name = str.substr(i_name1, i_name2 - i_name1 + 1);
		if(!b_quote && name.length() > 1 && name[name.length() - 1] == 'x') {
			MathFunction *f = CALCULATOR->getActiveFunction(name.substr(0, name.length() - 1));
			if(f && f->minargs() <= 1 && f->maxargs() != 0) return false;
			if(!f && CALCULATOR->getActiveVariable(name.substr(0, name.length() - 1))) return false;
		}
	}
	EvaluationOptions eo;
	eo.parse_options = po;
	if(CALCULATOR->hasWhereExpression(str, eo) && str.rfind(_("where"), i - 1) == string::npos && str.rfind("where", i - 1) == string::npos && str.rfind("/.", str.length() - 2) == string::npos) {
		return false;
	}
	size_t i2 = str.find(name, i);
	if(i2 != string::npos && str.rfind("#", i2 - 1) == string::npos) {
		if(b_quote) return false;
		string value = str.substr(i + 1, str.length() - (i + 1));
		CALCULATOR->parseComments(value);
		string stmp;
		CALCULATOR->separateWhereExpression(value, stmp, eo);
		CALCULATOR->separateToExpression(value, stmp, eo);
		CALCULATOR->parseSigns(value);
		size_t i2 = str.find(name, i);
		if(i2 != string::npos) {
			ExpressionItem *item1  = CALCULATOR->getActiveExpressionItem(name);
			ExpressionItem *item2  = item1 ? CALCULATOR->getActiveExpressionItem(name, item1) : NULL;
			if(item1) {
				MathStructure mtest;
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mtest, str.substr(i + 1, str.length() - (i + 1)), po);
				CALCULATOR->endTemporaryStopMessages();
				if(!b_func && item1->type() == TYPE_VARIABLE && !((Variable*) item1)->isKnown() && mtest.contains((Variable*) item1, true, true, false)) return false;
				else if(!b_func && item1->type() == TYPE_UNIT && mtest.contains((Unit*) item1, true, true, false)) return false;
				else if(b_func && item1->type() == TYPE_FUNCTION && mtest.containsFunction((MathFunction*) item1, true, true, false)) return false;
				if(!b_func && item2 && item2->type() == TYPE_VARIABLE && !((Variable*) item2)->isKnown() && mtest.contains((Variable*) item2, true, true, false)) return false;
				else if(!b_func && item2 && item2->type() == TYPE_UNIT && mtest.contains((Unit*) item2, true, true, false)) return false;
				else if(b_func && item2 && item2->type() == TYPE_FUNCTION && mtest.containsFunction((MathFunction*) item2, true, true, false)) return false;
			}
		}
	}
	str.insert(str.rfind(LEFT_PARENTHESIS, i) == string::npos ? i + 1 : i, ":");
	return true;
}

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == strlen(z) + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

string Calculator::calculateAndPrint(string str, int msecs, const EvaluationOptions &eo, const PrintOptions &po, std::string *parsed_expression) {
	return calculateAndPrint(str, msecs, eo, po, AUTOMATIC_FRACTION_OFF, AUTOMATIC_APPROXIMATION_OFF, parsed_expression, -1, NULL, false, 0, TAG_TYPE_HTML);
}

long int get_fixed_denominator2(const string &str, NumberFractionFormat &nff, bool b_minus, int frac) {
	long int fden = 0;
	if((frac > 0 && EQUALS_IGNORECASE_AND_LOCAL(str, "fraction", _("fraction"))) || (frac == 2 && str == "frac")) {
		fden = -1;
		if(b_minus) nff = FRACTION_FRACTIONAL;
		else nff = FRACTION_COMBINED;
	} else {
		if(str.length() > 2 && str[0] == '1' && str[1] == '/' && str.find_first_not_of(NUMBERS SPACES, 2) == string::npos) {
			fden = s2i(str.substr(2, str.length() - 2));
		} else if(str.length() > 1 && str[0] == '/' && str.find_first_not_of(NUMBERS SPACES, 1) == string::npos) {
			fden = s2i(str.substr(1, str.length() - 1));
		} else if(str == "3rds") {
			fden = 3;
		} else if(str == "halves") {
			fden = 2;
		} else if(str.length() > 3 && str.find("ths", str.length() - 3) != string::npos && str.find_first_not_of(NUMBERS SPACES) == str.length() - 3) {
			fden = s2i(str.substr(0, str.length() - 3));
		}
		if(fden > 1) {
			if(b_minus) nff = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
			else nff = FRACTION_COMBINED_FIXED_DENOMINATOR;
		}
	}
	return fden;
}
long int get_fixed_denominator(const string &str, NumberFractionFormat &nff, int frac, bool *has_sign) {
	size_t n = 0;
	if(str[0] == '-' || str[0] == '+') n = 1;
	if(has_sign) *has_sign = (n > 0);
	if(n > 0) return get_fixed_denominator2(str.substr(n, str.length() - n), nff, str[0] == '-', frac);
	return get_fixed_denominator2(str, nff, false, frac);
}

bool contains_fraction_q(const MathStructure &m) {
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction_q(m[i])) return true;
	}
	return false;
}

string Calculator::calculateAndPrint(string str, int msecs, const EvaluationOptions &eo, const PrintOptions &po, AutomaticFractionFormat auto_fraction, AutomaticApproximation auto_approx, std::string *parsed_expression, int max_length, bool *result_is_comparison, bool format, int colorize, int tagtype) {

	if(msecs > 0) startControl(msecs);

	PrintOptions printops = po;
	EvaluationOptions evalops = eo;

	if(auto_fraction != AUTOMATIC_FRACTION_OFF) printops.number_fraction_format = FRACTION_DECIMAL;
	if(auto_approx != AUTOMATIC_APPROXIMATION_OFF) evalops.approximation = APPROXIMATION_TRY_EXACT;

	MathStructure mstruct;
	bool do_bases = false, do_factors = false, do_pfe = false, do_calendars = false, do_expand = false, do_binary_prefixes = false, complex_angle_form = false, fraction_changed = false;

	gsub(ID_WRAP_LEFT, LEFT_PARENTHESIS, str);
	gsub(ID_WRAP_RIGHT, RIGHT_PARENTHESIS, str);

	string to_str = parseComments(str, evalops.parse_options);
	if(!to_str.empty() && str.empty()) {stopControl(); if(parsed_expression) {*parsed_expression = "";} return "";}

	// separate and handle string after "to"
	string from_str = str, str_conv;
	Number base_save;
	bool custom_base_set = false;
	bool fixed_fraction_has_sign = true;
	int save_bin = priv->use_binary_prefixes;
	long int save_fden = priv->fixed_denominator;
	ComplexNumberForm cnf = evalops.complex_number_form;
	bool delay_complex = false;
	bool had_to_expression = false;
	if(separateToExpression(from_str, to_str, evalops, true)) {
		remove_duplicate_blanks(to_str);
		string str_left;
		string to_str1, to_str2;
		had_to_expression = true;
		while(true) {
			separateToExpression(to_str, str_left, evalops, true);
			remove_blank_ends(to_str);
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", _("hexadecimal"))) {
				printops.base = BASE_HEXADECIMAL;
			} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
				printops.base = BASE_BINARY;
			} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", _("decimal"))) {
				printops.base = BASE_DECIMAL;
			} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
				printops.base = BASE_OCTAL;
			} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", _("duodecimal"))) {
				printops.base = BASE_DUODECIMAL;
			} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
				printops.base = BASE_DUODECIMAL;
				printops.duodecimal_symbols = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
				printops.base = BASE_ROMAN_NUMERALS;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, _("bijective"))) {
				printops.base = BASE_BIJECTIVE_26;
			} else if(equalsIgnoreCase(to_str, "bcd")) {
				printops.base = BASE_BINARY_DECIMAL;
			} else if(equalsIgnoreCase(to_str, "sexa") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "sexagesimal", _("sexagesimal"))) {
				printops.base = BASE_SEXAGESIMAL;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "2")) {
				printops.base = BASE_SEXAGESIMAL_2;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "3")) {
				printops.base = BASE_SEXAGESIMAL_3;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "longitude", _("longitude"))) {
				printops.base = BASE_LONGITUDE;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
				printops.base = BASE_LONGITUDE_2;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "latitude", _("latitude"))) {
				printops.base = BASE_LATITUDE;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
				printops.base = BASE_LATITUDE_2;
			} else if(equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float") || equalsIgnoreCase(to_str, "fp32")) {
				printops.base = BASE_FP32;
			} else if(equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double") || equalsIgnoreCase(to_str, "fp64")) {
				printops.base = BASE_FP64;
			} else if(equalsIgnoreCase(to_str, "binary16") || equalsIgnoreCase(to_str, "fp16")) {
				printops.base = BASE_FP16;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				printops.base = BASE_FP80;
			} else if(equalsIgnoreCase(to_str, "binary128") || equalsIgnoreCase(to_str, "fp128")) {
				printops.base = BASE_FP128;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
				printops.base = BASE_TIME;
			} else if(equalsIgnoreCase(to_str, "unicode")) {
				printops.base = BASE_UNICODE;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				printops.time_zone = TIME_ZONE_UTC;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				printops.base = BASE_BINARY;
				printops.binary_bits = s2i(to_str.substr(3));
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				printops.base = BASE_HEXADECIMAL;
				printops.binary_bits = s2i(to_str.substr(3));
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					error(true, _("Time zone parsing failed."), NULL);
				}
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = itz;
			} else if(to_str == "CET") {
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = 60;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "cartesian", _("cartesian")) || to_str == "rect") {
				cnf = COMPLEX_NUMBER_FORM_RECTANGULAR;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "exponential", _("exponential")) || to_str == "exp") {
				cnf = COMPLEX_NUMBER_FORM_EXPONENTIAL;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "polar", _("polar"))) {
				cnf = COMPLEX_NUMBER_FORM_POLAR;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "angle", _("angle")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "phasor", _("phasor"))) {
				cnf = COMPLEX_NUMBER_FORM_CIS;
				complex_angle_form = true;
			} else if(to_str == "cis") {
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors")) || equalsIgnoreCase(to_str, "factor")) {
				evalops.structuring = STRUCTURING_FACTORIZE;
				do_factors = true;
			}  else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction"))) {
				do_pfe = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases"))) {
				do_bases = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", _("calendars"))) {
				do_calendars = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "prefix", _("prefix"))) {
				evalops.parse_options.units_enabled = true;
				printops.use_prefixes_for_currencies = true;
				printops.use_prefixes_for_all_units = true;
				printops.use_unit_prefixes = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _c("units", "base"))) {
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", _("base"))) {
				if(to_str2 == "b26" || to_str2 == "B26") printops.base = BASE_BIJECTIVE_26;
				else if(equalsIgnoreCase(to_str2, "bcd")) printops.base = BASE_BINARY_DECIMAL;
				else if(equalsIgnoreCase(to_str2, "golden") || equalsIgnoreCase(to_str2, "golden ratio") || to_str2 == "φ") printops.base = BASE_GOLDEN_RATIO;
				else if(equalsIgnoreCase(to_str2, "unicode")) printops.base = BASE_UNICODE;
				else if(equalsIgnoreCase(to_str2, "supergolden") || equalsIgnoreCase(to_str2, "supergolden ratio") || to_str2 == "ψ") printops.base = BASE_SUPER_GOLDEN_RATIO;
				else if(equalsIgnoreCase(to_str2, "pi") || to_str2 == "π") printops.base = BASE_PI;
				else if(to_str2 == "e") printops.base = BASE_E;
				else if(to_str2 == "sqrt(2)" || to_str2 == "sqrt 2" || to_str2 == "sqrt2" || to_str2 == "√2") printops.base = BASE_SQRT2;
				else {
					EvaluationOptions eo = evalops;
					eo.parse_options.base = 10;
					MathStructure m = calculate(to_str2, eo);
					if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
						printops.base = m.number().intValue();
					} else {
						printops.base = BASE_CUSTOM;
						base_save = customOutputBase();
						custom_base_set = true;
						setCustomOutputBase(m.number());
					}
				}
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", _c("units", "mixed"))) {
				evalops.parse_options.units_enabled = true;
				evalops.auto_post_conversion = POST_CONVERSION_NONE;
				evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
			//decimal fraction
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimals", _("decimals"))) {
				printops.restrict_fraction_length = false;
				printops.number_fraction_format = FRACTION_DECIMAL;
				auto_fraction = AUTOMATIC_FRACTION_OFF;
				fraction_changed = true;
			} else {
				NumberFractionFormat nff = FRACTION_DECIMAL;
				string to_str2 = to_str;
				CALCULATOR->parseSigns(to_str2);
				long int fden = get_fixed_denominator(to_str2, nff, 1, &fixed_fraction_has_sign);
				if(fden != 0) {
					auto_fraction = AUTOMATIC_FRACTION_OFF;
					fraction_changed = true;
					printops.restrict_fraction_length = false;
					printops.number_fraction_format = nff;
					if(fden > 1) priv->fixed_denominator = fden;
				} else {
					// ? in front of unit expression is interpreted as a request for the optimal prefix.
					evalops.parse_options.units_enabled = true;
					if(to_str[0] == '?' || (to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'a' || to_str[0] == 'd'))) {
						printops.use_unit_prefixes = true;
						printops.use_prefixes_for_currencies = true;
						printops.use_prefixes_for_all_units = true;
						if(to_str[0] == 'a') printops.use_all_prefixes = true;
						else if(to_str[0] == 'd') priv->use_binary_prefixes = 0;
					} else if(to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'b') {
						// b? in front of unit expression: use binary prefixes
						do_binary_prefixes = true;
					}
					Unit *u = getActiveUnit(to_str);
					if(delay_complex != (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS) && u && u->baseUnit() == getRadUnit() && u->baseExponent() == 1) delay_complex = !delay_complex;
					// expression after "to" is by default interpreted as unit expression
					if(!str_conv.empty()) str_conv += " to ";
					str_conv += to_str;
				}
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		str = from_str;
		if(!str_conv.empty()) {
			str += " to ";
			str += str_conv;
		}
	}

	if(!delay_complex || (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS)) {
		evalops.complex_number_form = cnf;
		delay_complex = false;
	} else {
		evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	}

	// check for factor or expand instruction at front a expression
	size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
	if(i != string::npos) {
		to_str = str.substr(0, i);
		if(to_str == "factor" || EQUALS_IGNORECASE_AND_LOCAL(to_str, "factorize", _("factorize"))) {
			str = str.substr(i + 1);
			do_factors = true;
			evalops.structuring = STRUCTURING_FACTORIZE;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "expand", _("expand"))) {
			str = str.substr(i + 1);
			evalops.structuring = STRUCTURING_SIMPLIFY;
			do_expand = true;
		}
	}
	transform_expression_for_equals_save(str, evalops.parse_options);

	MathStructure parsed_struct;

	bool units_changed = false;

	// perform calculation
	mstruct = calculate(str, evalops, &parsed_struct);

	if(delay_complex) {
		evalops.complex_number_form = cnf;
		if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct.complexToCisForm(evalops);
		else if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct.complexToPolarForm(evalops);
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!had_to_expression && ((evalops.approximation == APPROXIMATION_EXACT && evalops.auto_post_conversion != POST_CONVERSION_NONE) || evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL) && ((parsed_struct.isMultiplication() && parsed_struct.size() == 2 && parsed_struct[0].isNumber() && parsed_struct[1].isUnit_exp() && parsed_struct.equals(mstruct)) || (parsed_struct.isNegate() && parsed_struct[0].isMultiplication() && parsed_struct[0].size() == 2 && parsed_struct[0][0].isNumber() && parsed_struct[0][1].isUnit_exp() && mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[1] == parsed_struct[0][1] && mstruct[0].isNumber() && parsed_struct[0][0].number() == -mstruct[0].number()) || (parsed_struct.isUnit_exp() && parsed_struct.equals(mstruct)))) {
		Unit *u = NULL;
		MathStructure *munit = NULL;
		if(mstruct.isMultiplication()) munit = &mstruct[1];
		else munit = &mstruct;
		if(munit->isUnit()) u = munit->unit();
		else u = (*munit)[0].unit();
		if(u && u->isCurrency()) {
			if(evalops.local_currency_conversion && getLocalCurrency() && u != getLocalCurrency()) {
				if(evalops.approximation == APPROXIMATION_EXACT) evalops.approximation = APPROXIMATION_TRY_EXACT;
				mstruct.set(convertToOptimalUnit(mstruct, evalops, true));
			}
		} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
			MathStructure mbak(mstruct);
			if(evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = getActiveUnit("oC");
					if(u) mstruct.set(convert(mstruct, u, evalops, true, false));
				} else if(munit->isUnit() && u->referenceName() == "oC") {
					u = getActiveUnit("oF");
					if(u) mstruct.set(convert(mstruct, u, evalops, true, false));
				} else {
					mstruct.set(convertToOptimalUnit(mstruct, evalops, true));
				}
			}
			if(evalops.approximation == APPROXIMATION_EXACT && (evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL || mstruct.equals(mbak))) {
				evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct.set(convertToBaseUnits(mstruct, evalops));
				else mstruct.set(convertToOptimalUnit(mstruct, evalops, true));
			}
		}
	}

	MathStructure mstruct_exact;
	mstruct_exact.setUndefined();
	if(!aborted() && (!do_calendars || !mstruct.isDateTime()) && !do_bases && !units_changed && auto_approx != AUTOMATIC_APPROXIMATION_OFF && (auto_approx == AUTOMATIC_APPROXIMATION_DUAL || printops.base == BASE_DECIMAL)) {
		bool b = true;
		if(msecs > 0) {
			long int usec, sec;
#ifndef CLOCK_MONOTONIC
			struct timeval tv;
			gettimeofday(&tv, NULL);
			usec = tv.tv_usec + msecs / 3L * 2L * 1000L;
			sec = tv.tv_sec;
			if(usec > 1000000L) {sec += usec / 1000000L; usec = usec % 1000000L;}
#else
			struct timespec tv;
			clock_gettime(CLOCK_MONOTONIC, &tv);
			usec = tv.tv_nsec / 1000L + msecs / 3L * 2L * 1000L;
			sec = tv.tv_sec;
			if(usec > 1000000L) {sec += usec / 1000000L; usec = usec % 1000000L;}
#endif
			if(sec > t_end.tv_sec || (sec == t_end.tv_sec && usec > t_end.tv_usec)) {
				b = false;
			}
		}
		if(b) {
			if(delay_complex) evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
			calculate_dual_exact(mstruct_exact, &mstruct, str, &parsed_struct, evalops, auto_approx, 0, max_length);
			if(aborted()) mstruct_exact.setUndefined();
			if(delay_complex && !mstruct_exact.isUndefined()) {
				evalops.complex_number_form = cnf;
				if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct_exact.complexToCisForm(evalops);
				else if(evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct_exact.complexToPolarForm(evalops);
			}
		}
	}
	if(aborted()) {
		stopControl();
		startControl(msecs / 5);
	}

	// handle "to factors", and "factor" or "expand" in front of the expression
	if(do_factors) {
		if((mstruct.isNumber() || mstruct.isVector()) && !fraction_changed) {
			auto_fraction = AUTOMATIC_FRACTION_OFF;
			printops.restrict_fraction_length = false;
			printops.number_fraction_format = FRACTION_FRACTIONAL;
		}
		mstruct.integerFactorize();
		mstruct_exact.integerFactorize();
	} else if(do_expand) {
		mstruct.expand(evalops, false);
	}

	// handle "to partial fraction"
	if(do_pfe) {
		mstruct.expandPartialFractions(evalops);
		mstruct_exact.expandPartialFractions(evalops);
	}

	if(!fixed_fraction_has_sign && printops.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR && !contains_fraction_q(mstruct)) printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;

	printops.allow_factorization = printops.allow_factorization || evalops.structuring == STRUCTURING_FACTORIZE || do_factors;

	bool exact_comparison = false;
	vector<string> alt_results;
	string result;
	bool tmp_approx = false;
	bool *save_is_approximate = printops.is_approximate;
	if(!printops.is_approximate) printops.is_approximate = &tmp_approx;

	if(do_calendars && mstruct.isDateTime()) {
		// handle "to calendars"
		bool b_fail;
		long int y, m, d;
#define PRINT_CALENDAR(x, c) if(!result.empty()) {result += "\n";} result += x; result += " "; b_fail = !dateToCalendar(*mstruct.datetime(), y, m, d, c); if(b_fail) {result += _("failed");} else {result += i2s(d); result += " "; result += monthName(m, c, true); result += " "; result += i2s(y);}
		PRINT_CALENDAR(string(_("Gregorian:")), CALENDAR_GREGORIAN);
		PRINT_CALENDAR(string(_("Hebrew:")), CALENDAR_HEBREW);
		PRINT_CALENDAR(string(_("Islamic:")), CALENDAR_ISLAMIC);
		PRINT_CALENDAR(string(_("Persian:")), CALENDAR_PERSIAN);
		PRINT_CALENDAR(string(_("Indian national:")), CALENDAR_INDIAN);
		PRINT_CALENDAR(string(_("Chinese:")), CALENDAR_CHINESE);
		long int cy, yc, st, br;
		chineseYearInfo(y, cy, yc, st, br);
		if(!b_fail) {result += " ("; result += chineseStemName(st); result += string(" "); result += chineseBranchName(br); result += ")";}
		PRINT_CALENDAR(string(_("Julian:")), CALENDAR_JULIAN);
		PRINT_CALENDAR(string(_("Revised julian:")), CALENDAR_MILANKOVIC);
		PRINT_CALENDAR(string(_("Coptic:")), CALENDAR_COPTIC);
		PRINT_CALENDAR(string(_("Ethiopian:")), CALENDAR_ETHIOPIAN);
		goto after_print;
	} else if(do_bases) {
		// handle "to bases"
		printops.base = BASE_BINARY;
		result = print(mstruct, 0, printops, format, colorize, tagtype);
		result += " = ";
		printops.base = BASE_OCTAL;
		result += print(mstruct, 0, printops, format, colorize, tagtype);
		result += " = ";
		printops.base = BASE_DECIMAL;
		result += print(mstruct, 0, printops, format, colorize, tagtype);
		result += " = ";
		printops.base = BASE_HEXADECIMAL;
		result += print(mstruct, 0, printops, format, colorize, tagtype);
		if(complex_angle_form) gsub(" cis ", "∠", result);
		goto after_print;
	} else if(do_binary_prefixes) {
		// b? in front of unit expression: use binary prefixes
		printops.use_unit_prefixes = true;
		// check for information units in unit expression: if found use only binary prefixes for information units
		int i = has_information_unit(mstruct);
		priv->use_binary_prefixes = (i > 0 ? 1 : 2);
		if(i == 1) {
			printops.use_denominator_prefix = false;
		} else if(i > 1) {
			printops.use_denominator_prefix = true;
		} else {
			printops.use_prefixes_for_currencies = true;
			printops.use_prefixes_for_all_units = true;
		}
	}

	if(result_is_comparison) *result_is_comparison = false;

	if(auto_fraction != AUTOMATIC_FRACTION_OFF || auto_approx != AUTOMATIC_APPROXIMATION_OFF) {
		print_dual(mstruct, str, parsed_struct, mstruct_exact, result, alt_results, printops, evalops, auto_fraction, auto_approx, complex_angle_form, &exact_comparison, true, format, colorize, tagtype, max_length, had_to_expression);
		if(!alt_results.empty()) {
			bool use_par = mstruct.isComparison() || mstruct.isLogicalAnd() || mstruct.isLogicalOr();
			str = result; result = "";
			size_t equals_length = 3;
			if(!printops.use_unicode_signs && (mstruct.isApproximate() || *printops.is_approximate)) equals_length += 1 + strlen(_("approx."));
			for(size_t i = 0; i < alt_results.size();) {
				if(max_length > 0 && unicode_length(str) + unicode_length(result) + unicode_length(alt_results[i]) + equals_length > (size_t) max_length) {
					alt_results.erase(alt_results.begin() + i);
				} else {
					if(i > 0) result += " = ";
					if(use_par) result += LEFT_PARENTHESIS;
					result += alt_results[i];
					if(use_par) result += RIGHT_PARENTHESIS;
					i++;
				}
			}
			if(alt_results.empty()) {
				result += str;
			} else {
				if(mstruct.isApproximate() || *printops.is_approximate) {
					if(printops.use_unicode_signs) {
						result += " " SIGN_ALMOST_EQUAL " ";
					} else {
						result += " = ";
						result += _("approx.");
						result += " ";
					}
				} else {
					result += " = ";
				}
				if(use_par) result += LEFT_PARENTHESIS;
				result += str;
				if(use_par) result += RIGHT_PARENTHESIS;
			}
		} else if(mstruct.isComparison() || mstruct.isLogicalOr() || mstruct.isLogicalAnd()) {
			if(result_is_comparison) {
				*result_is_comparison = true;
			} else {
				result.insert(0, LEFT_PARENTHESIS);
				result += RIGHT_PARENTHESIS;
			}
		}
	} else {

		if(po.spell_out_logical_operators && test_parsed_comparison(parsed_struct)) {
			if(mstruct.isZero()) {
				Variable *v = getActiveVariable("false");
				if(v) mstruct.set(v);
			} else if(mstruct.isOne()) {
				Variable *v = getActiveVariable("true");
				if(v) mstruct.set(v);
			}
		}

		// convert time units to hours when using time format
		if(printops.base == BASE_TIME) {
			bool b = false;
			if(mstruct.isUnit() && mstruct.unit()->baseUnit()->referenceName() == "s") {
				b = true;
			} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isUnit() && mstruct[1].unit()->baseUnit()->referenceName() == "s") {
				b = true;
			} else if(mstruct.isAddition() && mstruct.size() > 0) {
				b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					if(mstruct[i].isUnit() && mstruct[i].unit()->baseUnit()->referenceName() == "s") {}
					else if(mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][0].isNumber() && mstruct[i][1].isUnit() && mstruct[i][1].unit()->baseUnit()->referenceName() == "s") {}
					else {b = false; break;}
				}
			}
			if(b) {
				Unit *u = getActiveUnit("h");
				if(u) {
					mstruct.divide(u);
					mstruct.eval(evalops);
				}
			}
		}

		if(result_is_comparison && (mstruct.isComparison() || mstruct.isLogicalOr() || mstruct.isLogicalAnd())) *result_is_comparison = true;

		// do not display the default angle unit in trigonometric functions
		mstruct.removeDefaultAngleUnit(evalops);

		// format and print
		mstruct.format(printops);
		mstruct.removeDefaultAngleUnit(evalops);
		result = mstruct.print(printops, format, colorize, tagtype);

		// "to angle": replace "cis" with angle symbol
		if(complex_angle_form) gsub(" cis ", "∠", result);
	}

	after_print:

	// restore options
	if(custom_base_set) setCustomOutputBase(base_save);
	priv->use_binary_prefixes = save_bin;
	priv->fixed_denominator = save_fden;

	// output parsed value
	if(parsed_expression) {
		if(aborted()) {
			stopControl();
			startControl(msecs / 5);
		}
		PrintOptions po_parsed;
		po_parsed.preserve_format = true;
		po_parsed.show_ending_zeroes = false;
		po_parsed.lower_case_e = printops.lower_case_e;
		po_parsed.lower_case_numbers = printops.lower_case_numbers;
		po_parsed.base_display = printops.base_display;
		po_parsed.twos_complement = printops.twos_complement;
		po_parsed.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
		po_parsed.base = evalops.parse_options.base;
		Number nr_base;
		if(po_parsed.base == BASE_CUSTOM && (usesIntervalArithmetic() || customInputBase().isRational()) && (customInputBase().isInteger() || !customInputBase().isNegative()) && (customInputBase() > 1 || customInputBase() < -1)) {
			nr_base = customOutputBase();
			setCustomOutputBase(customInputBase());
		} else if(po_parsed.base == BASE_CUSTOM || (po_parsed.base < BASE_CUSTOM && !usesIntervalArithmetic() && po_parsed.base != BASE_UNICODE)) {
			po_parsed.base = 10;
			po_parsed.min_exp = 6;
			po_parsed.use_max_decimals = true;
			po_parsed.max_decimals = 5;
			po_parsed.preserve_format = false;
		}
		po_parsed.abbreviate_names = false;
		po_parsed.digit_grouping = printops.digit_grouping;
		po_parsed.use_unicode_signs = printops.use_unicode_signs;
		po_parsed.multiplication_sign = printops.multiplication_sign;
		po_parsed.division_sign = printops.division_sign;
		po_parsed.short_multiplication = false;
		po_parsed.excessive_parenthesis = true;
		po_parsed.improve_division_multipliers = false;
		po_parsed.restrict_to_parent_precision = false;
		po_parsed.spell_out_logical_operators = printops.spell_out_logical_operators;
		po_parsed.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		if(po_parsed.base == BASE_CUSTOM) {
			setCustomOutputBase(nr_base);
		}
		parsed_struct.format(po_parsed);
		*parsed_expression = parsed_struct.print(po_parsed, format, colorize, tagtype);
	}
	if(msecs > 0) stopControl();

	printops.is_approximate = save_is_approximate;
	if(po.is_approximate && (exact_comparison || !alt_results.empty())) *po.is_approximate = false;
	else if(po.is_approximate && mstruct.isApproximate()) *po.is_approximate = true;

	return result;
}
bool Calculator::calculate(MathStructure *mstruct, string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {

	mstruct->set(string(_("calculating...")), false, true);
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;

	// send calculation command to calculation thread
	expression_to_calculate = str;
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	tmp_rpn_mstruct = NULL;
	tmp_parsedstruct = parsed_struct;
	tmp_tostruct = to_struct;
	tmp_maketodivision = make_to_division;
	if(!calculate_thread->write(true)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}

	// check time while calculation proceeds
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		if(!abort()) mstruct->setAborted();
		return false;
	}
	return true;
}
bool Calculator::calculate(MathStructure *mstruct, int msecs, const EvaluationOptions &eo, string to_str) {

	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;

	// send calculation command to calculation thread
	expression_to_calculate = "";
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	tmp_rpn_mstruct = NULL;
	tmp_parsedstruct = NULL;
	if(!to_str.empty()) tmp_tostruct = new MathStructure(to_str);
	else tmp_tostruct = NULL;
	tmp_maketodivision = false;
	if(!calculate_thread->write(false)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}

	// check time while calculation proceeds
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(had_msecs && b_busy) {
		if(!abort()) mstruct->setAborted();
		return false;
	}
	return true;
}
bool Calculator::hasToExpression(const string &str, bool allow_empty_from) const {
	if(str.empty()) return false;
	size_t i = str.rfind("->");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = str.rfind("→");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = str.rfind(SIGN_MINUS ">");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = allow_empty_from ? 0 : 1;
	while(true) {
		// dingbat arrows
		i = str.find("\xe2\x9e", i);
		if(i == string::npos || i >= str.length() - 2) break;
		if((unsigned char) str[i + 2] >= 0x94 && (unsigned char) str[i + 2] <= 0xbf) return true;
	}
	i = allow_empty_from ? 0 : 1;
	size_t i2 = i;
	int l = 2;
	while(true) {
		i2 = str.find(_("to"), i);
		i = str.find("to", i);
		if(i2 != string::npos && (i == string::npos || i2 < i)) {l = strlen(_("to")); i = i2;}
		else l = 2;
		if(i == string::npos) break;
		if(((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
		i++;
	}
	return false;
}
bool Calculator::hasToExpression(const string &str, bool allow_empty_from, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	if(str.empty()) return false;
	size_t i = str.rfind("->");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = str.rfind("→");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = str.rfind(SIGN_MINUS ">");
	if(i != string::npos && (allow_empty_from || i > 0)) return true;
	i = allow_empty_from ? 0 : 1;
	while(true) {
		// dingbat arrows
		i = str.find("\xe2\x9e", i);
		if(i == string::npos || i >= str.length() - 2) break;
		if((unsigned char) str[i + 2] >= 148 && (unsigned char) str[i + 2] <= 191) return true;
	}
	i = allow_empty_from ? 0 : 1;
	size_t i2 = i;
	int l = 2;
	while(true) {
		i2 = str.find(_("to"), i);
		i = str.find("to", i);
		if(i2 != string::npos && (i == string::npos || i2 < i)) {l = strlen(_("to")); i = i2;}
		else l = 2;
		if(i == string::npos) break;
		if(((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
		i++;
	}
	return false;
}
bool Calculator::separateToExpression(string &str, string &to_str, const EvaluationOptions &eo, bool keep_modifiers, bool allow_empty_from) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	to_str = "";
	if(str.empty()) return false;
	size_t i = 1, i2, l_arrow = 2;
	if(allow_empty_from) i = 0;
	size_t i_arrow = str.find("->", i);
	i2 = str.find("→", i);
	if(i2 != string::npos && i2 < i_arrow) {i_arrow = i2; l_arrow = 3;}
	i2 = str.find(SIGN_MINUS ">", i);
	if(i2 != string::npos && i2 < i_arrow) {i_arrow = i2; l_arrow = 4;}
	while(true) {
		// dingbat arrows
		i = str.find("\xe2\x9e", i); 
		if(i == string::npos || (i_arrow != string::npos && i > i_arrow) || i >= str.length() - 2) break;
		if((unsigned char) str[i + 2] >= 148 && (unsigned char) str[i + 2] <= 191) {i_arrow = i; l_arrow = 3; break;}
		i += 3;
	}
	i = 0;
	int l = 2;
	while(true) {
		i2 = str.find(_("to"), i);
		i = str.find("to", i);
		l = 2;
		bool b_arrow = false;
		if(i2 != string::npos && (i == string::npos || i2 < i)) {l = strlen(_("to")); i = i2;}
		if(i_arrow != string::npos && (i == string::npos || i_arrow < i)) {l = l_arrow; i = i_arrow; b_arrow = true;}
		if(i == string::npos) break;
		if(b_arrow || (((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l]))) {
			to_str = str.substr(i + l , str.length() - i - l);
			if(!b_arrow && to_str.empty()) return false;
			remove_blank_ends(to_str);
			if(!to_str.empty()) {
				if(to_str.rfind(SIGN_MINUS, 0) == 0) {
					to_str.replace(0, strlen(SIGN_MINUS), MINUS);
				}
				if(!keep_modifiers && (to_str[0] == '0' || to_str[0] == '?' || to_str[0] == '+' || to_str[0] == '-')) {
					to_str = to_str.substr(1, to_str.length() - 1);
					remove_blank_ends(to_str);
				} else if(!keep_modifiers && to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
					to_str = to_str.substr(2, to_str.length() - 2);
					remove_blank_ends(to_str);
				}
			}
			str = str.substr(0, i);
			remove_blank_ends(str);
			return true;
		}
		i++;
	}
	return false;
}

string Calculator::parseToExpression(string to_str, EvaluationOptions &evalops, PrintOptions &printops, Number *custom_base, int *binary_prefixes, bool *complex_angle_form, bool *do_factors, bool *do_pfe, bool *do_calendars, bool *do_bases) const {
	remove_duplicate_blanks(to_str);
	string str_left, str_conv;
	string to_str1, to_str2;
	bool fixed_fraction_has_sign = true;
	if(custom_base) custom_base->clear();
	if(binary_prefixes) *binary_prefixes = -1;
	if(do_factors) *do_factors = false;
	if(do_pfe) *do_pfe = false;
	if(do_calendars) *do_calendars = false;
	if(do_bases) *do_bases = false;
	while(true) {
		separateToExpression(to_str, str_left, evalops, true);
		remove_blank_ends(to_str);
		size_t ispace = to_str.find_first_of(SPACES);
		if(ispace != string::npos) {
			to_str1 = to_str.substr(0, ispace);
			remove_blank_ends(to_str1);
			to_str2 = to_str.substr(ispace + 1);
			remove_blank_ends(to_str2);
		}
		if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", _("hexadecimal"))) {
			printops.base = BASE_HEXADECIMAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
			printops.base = BASE_BINARY;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", _("decimal"))) {
			printops.base = BASE_DECIMAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
			printops.base = BASE_OCTAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", _("duodecimal"))) {
			printops.base = BASE_DUODECIMAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
			printops.base = BASE_DUODECIMAL;
			printops.duodecimal_symbols = true;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
			printops.base = BASE_ROMAN_NUMERALS;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, _("bijective"))) {
			printops.base = BASE_BIJECTIVE_26;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "bcd")) {
			printops.base = BASE_BINARY_DECIMAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "sexa") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "sexagesimal", _("sexagesimal"))) {
			printops.base = BASE_SEXAGESIMAL;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "2")) {
			printops.base = BASE_SEXAGESIMAL_2;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", _("sexagesimal"), "3")) {
			printops.base = BASE_SEXAGESIMAL_3;
			if(custom_base) custom_base->clear();
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "longitude", _("longitude"))) {
			printops.base = BASE_LONGITUDE;
			if(custom_base) custom_base->clear();
		} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", _("longitude"), "2")) {
			printops.base = BASE_LONGITUDE_2;
			if(custom_base) custom_base->clear();
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "latitude", _("latitude"))) {
			printops.base = BASE_LATITUDE;
			if(custom_base) custom_base->clear();
		} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", _("latitude"), "2")) {
			printops.base = BASE_LATITUDE_2;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float") || equalsIgnoreCase(to_str, "fp32")) {
			printops.base = BASE_FP32;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double") || equalsIgnoreCase(to_str, "fp64")) {
			printops.base = BASE_FP64;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "binary16") || equalsIgnoreCase(to_str, "fp16")) {
			printops.base = BASE_FP16;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "fp80")) {
			printops.base = BASE_FP80;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "binary128") || equalsIgnoreCase(to_str, "fp128")) {
			printops.base = BASE_FP128;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
			printops.base = BASE_TIME;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "unicode")) {
			printops.base = BASE_UNICODE;
			if(custom_base) custom_base->clear();
		} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
			printops.time_zone = TIME_ZONE_UTC;
		} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
			printops.base = BASE_BINARY;
			if(custom_base) custom_base->clear();
			printops.binary_bits = s2i(to_str.substr(3));
		} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
			printops.base = BASE_HEXADECIMAL;
			printops.binary_bits = s2i(to_str.substr(3));
			if(custom_base) custom_base->clear();
		} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
			to_str = to_str.substr(3);
			remove_blanks(to_str);
			bool b_minus = false;
			if(to_str[0] == '+') {
				to_str.erase(0, 1);
			} else if(to_str[0] == '-') {
				b_minus = true;
				to_str.erase(0, 1);
			} else if(to_str.find(SIGN_MINUS) == 0) {
				b_minus = true;
				to_str.erase(0, strlen(SIGN_MINUS));
			}
			unsigned int tzh = 0, tzm = 0;
			int itz = 0;
			if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
				itz = tzh * 60 + tzm;
				if(b_minus) itz = -itz;
			} else {
				CALCULATOR->error(true, _("Time zone parsing failed."), NULL);
			}
			printops.time_zone = TIME_ZONE_CUSTOM;
			printops.custom_time_zone = itz;
		} else if(to_str == "CET") {
			printops.time_zone = TIME_ZONE_CUSTOM;
			printops.custom_time_zone = 60;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "cartesian", _("cartesian")) || to_str == "rect") {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "exponential", _("exponential")) || to_str == "exp") {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "polar", _("polar"))) {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "angle", _("angle")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "phasor", _("phasor"))) {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
			if(complex_angle_form) *complex_angle_form = true;
		} else if(to_str == "cis") {
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
		} else if(do_factors && (EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors")) || equalsIgnoreCase(to_str, "factor"))) {
			evalops.structuring = STRUCTURING_FACTORIZE;
			if(do_factors) *do_factors = true;
		}  else if(do_pfe && (equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction")))) {
			*do_pfe = true;
		} else if(do_bases && (EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases")))) {
			*do_bases = true;
		} else if(do_calendars && (EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", _("calendars")))) {
			*do_calendars = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
			str_conv = "";
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "prefix", _("prefix"))) {
			evalops.parse_options.units_enabled = true;
			printops.use_prefixes_for_currencies = true;
			printops.use_prefixes_for_all_units = true;
			printops.use_unit_prefixes = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _c("units", "base"))) {
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_BASE;
			str_conv = "";
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", _("base"))) {
			if(custom_base) custom_base->clear();
			if(to_str2 == "b26" || to_str2 == "B26") printops.base = BASE_BIJECTIVE_26;
			else if(equalsIgnoreCase(to_str2, "bcd")) printops.base = BASE_BINARY_DECIMAL;
			else if(equalsIgnoreCase(to_str2, "golden") || equalsIgnoreCase(to_str2, "golden ratio") || to_str2 == "φ") printops.base = BASE_GOLDEN_RATIO;
			else if(equalsIgnoreCase(to_str2, "unicode")) printops.base = BASE_UNICODE;
			else if(equalsIgnoreCase(to_str2, "supergolden") || equalsIgnoreCase(to_str2, "supergolden ratio") || to_str2 == "ψ") printops.base = BASE_SUPER_GOLDEN_RATIO;
			else if(equalsIgnoreCase(to_str2, "pi") || to_str2 == "π") printops.base = BASE_PI;
			else if(to_str2 == "e") printops.base = BASE_E;
			else if(to_str2 == "sqrt(2)" || to_str2 == "sqrt 2" || to_str2 == "sqrt2" || to_str2 == "√2") printops.base = BASE_SQRT2;
			else {
				EvaluationOptions eo = evalops;
				eo.parse_options.base = 10;
				MathStructure m = CALCULATOR->calculate(to_str2, eo);
				if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
					printops.base = m.number().intValue();
				} else {
					if(custom_base) custom_base->set(m.number());
				}
			}
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", _c("units", "mixed"))) {
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_NONE;
			evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
		//decimal fraction
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimals", _("decimals"))) {
			printops.restrict_fraction_length = false;
			printops.number_fraction_format = FRACTION_DECIMAL;
		} else {
			NumberFractionFormat nff = FRACTION_DECIMAL;
			string to_str2 = to_str;
			CALCULATOR->parseSigns(to_str2);
			long int fden = get_fixed_denominator(to_str2, nff, 1, &fixed_fraction_has_sign);
			if(fden != 0) {
				printops.restrict_fraction_length = false;
				printops.number_fraction_format = nff;
				if(fden > 1) priv->fixed_denominator = fden;
			} else {
				// ? in front of unit expression is interpreted as a request for the optimal prefix.
				evalops.parse_options.units_enabled = true;
				if(to_str[0] == '?' || (to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'a' || to_str[0] == 'd'))) {
					printops.use_unit_prefixes = true;
					printops.use_prefixes_for_currencies = true;
					printops.use_prefixes_for_all_units = true;
					if(to_str[0] == 'a') {
						printops.use_all_prefixes = true;
					} else if(to_str[0] == 'd') {
						if(*binary_prefixes) *binary_prefixes = 0;
					}
				} else if(to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'b') {
					// b? in front of unit expression: use binary prefixes
					if(*binary_prefixes) *binary_prefixes = 2;
				}
				// expression after "to" is by default interpreted as unit expression
				if(!str_conv.empty()) str_conv += " to ";
				str_conv += to_str;
			}
		}
		if(str_left.empty()) break;
		to_str = str_left;
	}
	return str_conv;
}
size_t find_outside_enclosures(const string &str, char c, size_t i = 0) {
	int par = 0, bra = 0;
	bool quo1 = false, quo2 = false;
	for(; i < str.length(); i++) {
		switch(str[i]) {
			case '(': {if(!quo1 && !quo2) par++; break;}
			case ')': {if(par > 0 && !quo1 && !quo2) par--; break;}
			case '[': {if(!quo1 && !quo2) bra++; break;}
			case ']': {if(bra > 0 && !quo1 && !quo2) bra--; break;}
			case '\"': {if(!quo2) quo1 = !quo1; break;}
			case '\'': {if(!quo1) quo2 = !quo2; break;}
			default: {
				if(str[i] == c && !quo1 && !quo2 && bra == 0 && par == 0) return i;
				break;
			}
		}
	}
	return string::npos;
}
size_t rfind_outside_enclosures(const string &str, char c) {
	int par = 0, bra = 0;
	bool quo1 = false, quo2 = false;
	size_t pos = string::npos;
	for(size_t i = 0; i < str.length(); i++) {
		switch(str[i]) {
			case '(': {if(!quo1 && !quo2) par++; break;}
			case ')': {if(par > 0 && !quo1 && !quo2) par--; break;}
			case '[': {if(!quo1 && !quo2) bra++; break;}
			case ']': {if(bra > 0 && !quo1 && !quo2) bra--; break;}
			case '\"': {if(!quo2) quo1 = !quo1; break;}
			case '\'': {if(!quo1) quo2 = !quo2; break;}
			default: {
				if(str[i] == c && !quo1 && !quo2 && bra == 0 && par == 0) pos = i;
				break;
			}
		}
	}
	return pos;
}
bool Calculator::hasWhereExpression(const string &str, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	if(str.empty()) return false;
	size_t i = str.length() - 1, i2 = i;
	int l = 2;
	while(i != 0) {
		//"where"-operator
		i2 = str.rfind(_("where"), i - 1);
		i = str.rfind("where", i - 1);
		if(i2 != string::npos && (i == string::npos || i < i2)) {l = strlen(_("where")); i = i2;}
		else l = 5;
		if(i == string::npos) break;
		if(i > 0 && is_in(SPACES, str[i - 1]) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
	}
	if((i = str.rfind("/.", str.length() - 2)) != string::npos && eo.parse_options.base >= 2 && eo.parse_options.base <= 10 && (str[i + 2] < '0' || str[i + 2] > '9')) return true;
	size_t i4 = rfind_outside_enclosures(str, COMMA_CH);
	if(i4 == string::npos || i4 < 3) return false;
	i = 0;
	i2 = 0;
	bool var_found = false;
	while(true) {
		i2 = str.find_first_not_of(SPACES, i2);
		if(i2 == string::npos) return false;
		i = str.find_first_of("<>=\xe2", i2 + 1);
		if(i == string::npos || i > i4) return false;
		while(str[i] == '\xe2') {
			if(str[i + 1] == '\x89' && (str[i + 2] == '\xa4' || str[i + 2] == '\xa5' || str[i + 2] == '\xa0')) break;
			i = str.find_first_of("<>=\xe2", i + 1);
			if(i == string::npos || i > i4) return false;
		}
		if(str[i] == '=') {
			if(str[i - 1] == '!') i--;
			else if(str[i - 1] == ':' || str[i + 1] == ':' || str[i + 1] == '=') return false;
		}
		if(i == i2) return false;
		i = str.find_last_not_of(SPACES, i - 1);
		size_t i5 = i2;
		i2 = find_outside_enclosures(str, COMMA_CH, i + 1);
		if(i2 == string::npos || i2 == str.length() - 1) return false;
		string sname = str.substr(i5, i - i5 + 1);
		parseSigns(sname);
		if(sname.length() < 3 || ((sname[0] != '\"' && sname[0] != '\'') || sname[sname.length() - 1] != sname[0])) {
			if(sname.length() >= 2 && sname[0] == '\\') {
				if((unsigned char) sname[1] >= 0xC0) {
					for(size_t i3 = 2; i3 < sname.length(); i3++) {
						if((unsigned char) sname[i3] >= 0xC0 || (signed char) sname[i3] > 0) return false;
					}
				} else if(i != 2 || sname[1] > 'z' || sname[1] < 'A' || (sname[1] > 'Z' && sname[1] < 'a')) {
					return false;
				}
			} else if(!variableNameIsValid(sname)) {
				return false;
			}
		}
		if(!var_found) {
			if(sname[0] == '\"' || sname[0] == '\'' || sname[0] == '\\') {
				if(str.find(str.substr(i5, i - i5 + 1), i4 + 1) != string::npos) var_found = true;
			} else {
				string svalue = str.substr(i4 + 1, str.length() - (i4 + 1));
				parseSigns(svalue);
				size_t i7 = 0;
				while(!var_found) {
					size_t i3 = svalue.find(sname, i7);
					if(i3 == string::npos) break;
					size_t i6 = i3;
					i7 = i6 + sname.length() - 1;
					while(i6 > 0 && is_not_in(NOT_IN_NAMES NUMBERS, svalue[i6 - 1])) i6--;
					while(i7 < svalue.length() - 1 && is_not_in(NOT_IN_NAMES NUMBERS, svalue[i7 + 1])) i7++;
					if((i6 == i3 && i7 == i6 + sname.length() - 1) || !CALCULATOR->getActiveExpressionItem(svalue.substr(i6, i7 - i6 + 1))) {
						var_found = true;
					}
					i7++;
				}
			}
		}
		if(i2 >= i4) break;
		i2++;
	}
	if(i2 != i4 || !var_found) return false;
	return var_found;
}
bool Calculator::separateWhereExpression(string &str, string &to_str, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	to_str = "";
	size_t i = 0;
	if((i = str.rfind("/.", str.length() - 2)) != string::npos && i != str.length() - 2 && eo.parse_options.base >= 2 && eo.parse_options.base <= 10 && (str[i + 2] < '0' || str[i + 2] > '9')) {
		to_str = str.substr(i + 2 , str.length() - i - 2);
	} else {
		i = str.length() - 1;
		size_t i2 = i;
		int l = 5;
		while(i != 0) {
			i2 = str.rfind(_("where"), i - 1);
			i = str.rfind("where", i - 1);
			if(i2 != string::npos && (i == string::npos || i < i2)) {l = strlen(_("where")); i = i2;}
			else l = 5;
			if(i == string::npos) break;
			if(i > 0 && is_in(SPACES, str[i - 1]) && i + l < str.length() && is_in(SPACES, str[i + l])) {
				to_str = str.substr(i + l , str.length() - i - l);
				break;
			}
		}
	}
	if(!to_str.empty()) {
		str = str.substr(0, i);
		remove_blank_ends(str);
		remove_blank_ends(to_str);
		parseSigns(to_str);
		if(to_str.find("&&") == string::npos) {
			i = 0;
			while(true) {
				i = find_outside_enclosures(to_str, COMMA_CH, i);
				if(i == string::npos) break;
				to_str.replace(i, 1, LOGICAL_AND);
				i++;
			}
		}
		return true;
	}

	size_t i4 = rfind_outside_enclosures(str, COMMA_CH);
	if(i4 == string::npos || i4 < 3) return false;
	i = 0;
	size_t i2 = 0;
	bool var_found = false;
	while(true) {
		i2 = str.find_first_not_of(SPACES, i2);
		if(i2 == string::npos) return false;
		i = str.find_first_of("<>=\xe2", i2 + 1);
		if(i == string::npos || i > i4) return false;
		while(str[i] == '\xe2') {
			if(str[i + 1] == '\x89' && (str[i + 2] == '\xa4' || str[i + 2] == '\xa5' || str[i + 2] == '\xa0')) break;
			i = str.find_first_of("<>=\xe2", i + 1);
			if(i == string::npos || i > i4) return false;
		}
		if(str[i] == '=') {
			if(str[i - 1] == '!') i--;
			else if(str[i - 1] == ':' || str[i + 1] == ':' || str[i + 1] == '=') return false;
		}
		if(i == i2) return false;
		i = str.find_last_not_of(SPACES, i - 1);
		size_t i5 = i2;
		i2 = find_outside_enclosures(str, COMMA_CH, i + 1);
		if(i2 == string::npos || i2 == str.length() - 1) return false;
		string sname = str.substr(i5, i - i5 + 1);
		parseSigns(sname);
		if(sname.length() < 3 || ((sname[0] != '\"' && sname[0] != '\'') || sname[sname.length() - 1] != sname[0])) {
			if(sname.length() >= 2 && sname[0] == '\\') {
				if((unsigned char) sname[1] >= 0xC0) {
					for(size_t i3 = 2; i3 < sname.length(); i3++) {
						if((unsigned char) sname[i3] >= 0xC0 || (signed char) sname[i3] > 0) return false;
					}
				} else if(i != 2 || sname[1] > 'z' || sname[1] < 'A' || (sname[1] > 'Z' && sname[1] < 'a')) {
					return false;
				}
			} else if(!variableNameIsValid(sname)) {
				return false;
			}
		}
		if(!var_found) {
			if(sname[0] == '\"' || sname[0] == '\'' || sname[0] == '\\') {
				if(str.find(str.substr(i5, i - i5 + 1), i4 + 1) != string::npos) var_found = true;
			} else {
				string svalue = str.substr(i4 + 1, str.length() - (i4 + 1));
				parseSigns(svalue);
				size_t i7 = 0;
				while(!var_found) {
					size_t i3 = svalue.find(sname, i7);
					if(i3 == string::npos) break;
					size_t i6 = i3;
					i7 = i6 + sname.length() - 1;
					while(i6 > 0 && is_not_in(NOT_IN_NAMES NUMBERS, svalue[i6 - 1])) i6--;
					while(i7 < svalue.length() - 1 && is_not_in(NOT_IN_NAMES NUMBERS, svalue[i7 + 1])) i7++;
					if((i6 == i3 && i7 == i6 + sname.length() - 1) || !CALCULATOR->getActiveExpressionItem(svalue.substr(i6, i7 - i6 + 1))) {
						var_found = true;
					}
					i7++;
				}
			}
		}
		if(i2 >= i4) break;
		i2++;
	}
	if(i2 != i4 || !var_found) return false;
	to_str = str.substr(0, i4);
	if(i4 == str.length() - 1) str = "";
	else str = str.substr(i4 + 1, str.length() - i4 + 1);
	remove_blank_ends(str);
	remove_blank_ends(to_str);
	parseSigns(to_str);
	i = 0;
	while(true) {
		i = find_outside_enclosures(to_str, COMMA_CH, i);
		if(i == string::npos) break;
		to_str.replace(i, 1, LOGICAL_AND);
		i++;
	}
	return true;
}
bool calculate_rand(MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_RAND || mstruct.function()->id() == FUNCTION_ID_RANDN || mstruct.function()->id() == FUNCTION_ID_RAND_POISSON)) {
		mstruct.unformat(eo);
		mstruct.calculateFunctions(eo, false);
		return true;
	}
	bool ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(calculate_rand(mstruct[i], eo)) {
			ret = true;
			mstruct.childUpdated(i + 1);
		}
	}
	return ret;
}
bool calculate_ans(MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.isFunction() && (mstruct.function()->hasName("answer") || mstruct.function()->hasName("expression"))) {
		mstruct.unformat(eo);
		mstruct.calculateFunctions(eo, false);
		return true;
	} else if(mstruct.isVariable() && mstruct.variable()->isKnown() && (mstruct.variable()->referenceName() == "ans" || (mstruct.variable()->referenceName().length() == 4 && mstruct.variable()->referenceName().substr(0, 3) == "ans" && is_in(NUMBERS, mstruct.variable()->referenceName()[3])))) {
		mstruct.set(((KnownVariable*) mstruct.variable())->get(), true);
		return true;
	}
	bool ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(calculate_ans(mstruct[i], eo)) {
			mstruct.childUpdated(i + 1);
			ret = true;
		}
	}
	return ret;
}
bool handle_where_expression(MathStructure &m, MathStructure &mstruct, const EvaluationOptions &eo, vector<Variable*>& vars, bool empty_func, bool do_eval = true) {
	if(m.isComparison()) {
		if(m.comparisonType() == COMPARISON_EQUALS) {
			// x=y
			if(m[0].size() > 0 && do_eval) {
				// not a single variable (or empty function) on the left side of the comparison: perform calculation
				MathStructure m2(m);
				MathStructure xvar = m[0].find_x_var();
				EvaluationOptions eo2 = eo;
				eo2.isolate_x = true;
				if(!xvar.isUndefined()) eo2.isolate_var = &xvar;
				m2.eval(eo2);
				if(m2.isComparison()) return handle_where_expression(m2, mstruct, eo, vars, false, false);
			}
			if(m[0].isFunction() && m[1].isFunction() && (m[0].size() == 0 || (empty_func && m[0].function()->minargs() == 0)) && (m[1].size() == 0 || (empty_func && m[1].function()->minargs() == 0))) {
				// if left value is a function without any arguments, do function replacement
				if(!replace_function(mstruct, m[0].function(), m[1].function(), eo)) CALCULATOR->error(false, _("Original value (%s) was not found."), (m[0].function()->name() + "()").c_str(), NULL);
			} else {
				if(mstruct.countOccurrences(m[0], true) > 1) {

					// make sure that only a single random value is used
					calculate_rand(m[1], eo);

					if(m[1].containsInterval(true, false, false, 0, true)) {
						// replace interval with variable if multiple occurrences if original value
						MathStructure mv(m[1]);
						replace_f_interval(mv, eo);
						replace_intervals_f(mv);
						if(!mstruct.replace(m[0], mv, false, false, true)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
					} else {
						if(!mstruct.replace(m[0], m[1], false, false, true)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
					}
				} else {
					if(!mstruct.replace(m[0], m[1], false, false, true)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
				}
			}
			return true;
		} else if(m[0].isSymbolic() || (m[0].isVariable() && !m[0].variable()->isKnown())) {
			// inequality found
			// unknown variable range specification (e.g. x>0)
			// right hand side value must be a non-complex number
			if(!m[1].isNumber()) m[1].eval(eo);
			if(m[1].isNumber() && !m[1].number().hasImaginaryPart()) {
				Assumptions *ass = NULL;
				// search for assumptions from previous "where" replacements
				for(size_t i = 0; i < vars.size(); i++) {
					if(!vars[i]->isKnown() && (m[0] == vars[i] || (m[0].isSymbolic() && format_and_print(m[0]) == vars[i]->name()))) {
						ass = ((UnknownVariable*) vars[i])->assumptions();
						break;
					}
				}
				// can only handle not equals if value is zero
				if((m.comparisonType() != COMPARISON_NOT_EQUALS || (!ass && m[1].isZero()))) {
					if(ass) {
						// change existing assumptions
						if(m.comparisonType() == COMPARISON_EQUALS_GREATER) {
							if(!ass->min() || (*ass->min() < m[1].number())) {
								ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(true);
								return true;
							} else if(*ass->min() >= m[1].number()) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_EQUALS_LESS) {
							if(!ass->max() || (*ass->max() > m[1].number())) {
								ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(true);
								return true;
							} else if(*ass->max() <= m[1].number()) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_GREATER) {
							if(!ass->min() || (ass->includeEqualsMin() && *ass->min() <= m[1].number()) || (!ass->includeEqualsMin() && *ass->min() < m[1].number())) {
								ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(false);
								return true;
							} else if((ass->includeEqualsMin() && *ass->min() > m[1].number()) || (!ass->includeEqualsMin() && *ass->min() >= m[1].number())) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_LESS) {
							if(!ass->max() || (ass->includeEqualsMax() && *ass->max() >= m[1].number()) || (!ass->includeEqualsMax() && *ass->max() > m[1].number())) {
								ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(false);
								return true;
							} else if((ass->includeEqualsMax() && *ass->max() < m[1].number()) || (!ass->includeEqualsMax() && *ass->max() <= m[1].number())) {
								return true;
							}
						}
					} else {
						// create a new unknown variable and modify the assumptions
						UnknownVariable *var = new UnknownVariable("", format_and_print(m[0]));
						ass = new Assumptions();
						if(m[1].isZero()) {
							if(m.comparisonType() == COMPARISON_EQUALS_GREATER) ass->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
							else if(m.comparisonType() == COMPARISON_EQUALS_LESS) ass->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
							else if(m.comparisonType() == COMPARISON_GREATER) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
							else if(m.comparisonType() == COMPARISON_LESS) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
							else if(m.comparisonType() == COMPARISON_NOT_EQUALS) ass->setSign(ASSUMPTION_SIGN_NONZERO);
						} else {
							if(m.comparisonType() == COMPARISON_EQUALS_GREATER) {ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(true);}
							else if(m.comparisonType() == COMPARISON_EQUALS_LESS) {ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(true);}
							else if(m.comparisonType() == COMPARISON_GREATER) {ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(false);}
							else if(m.comparisonType() == COMPARISON_LESS) {ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(false);}
						}
						var->setAssumptions(ass);
						vars.push_back(var);
						MathStructure u_var(var);
						if(!mstruct.replace(m[0], u_var, false, false, true)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
						return true;
					}
				}
			}
		} else if(do_eval) {
			// inequality without a single variable on the left side: calculate expression and try again
			MathStructure xvar = m[0].find_x_var();
			EvaluationOptions eo2 = eo;
			eo2.isolate_x = true;
			if(!xvar.isUndefined()) eo2.isolate_var = &xvar;
			m.eval(eo2);
			return handle_where_expression(m, mstruct, eo, vars, false, false);
		}
	} else if(m.isLogicalAnd()) {
		// logical and (e.g. x=2&&y=3): perform multiple "where" replacements
		bool ret = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(!handle_where_expression(m[i], mstruct, eo, vars, empty_func, do_eval)) ret = false;
		}
		return ret;
	}
	CALCULATOR->error(true, _("Unhandled \"where\" expression: %s"), format_and_print(m).c_str(), NULL);
	return false;
}
void replace_unregistered_variables(MathStructure &m) {
	if(m.isVariable() && m.variable()->isKnown() && !m.variable()->isRegistered()) {
		m.set(((KnownVariable*) m.variable())->get());
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_unregistered_variables(m[i]);
	}
}


void Calculator::parseExpressionAndWhere(MathStructure *mstruct, MathStructure *mwhere, string str, string str_where, const ParseOptions &po) {
	vector<Variable*> where_vars;
	vector<bool> repeat;
	vector<vector<size_t>::iterator> where_its;
	vector<string> wheres;
	mwhere->clear();
	if(!str_where.empty()) {
		parseSigns(str_where, false);
		remove_duplicate_blanks(str_where);
		int par = 0, bra = 0;
		bool quo1 = false, quo2 = false;
		size_t i = 0;
		for(size_t i3 = 0; i3 < str_where.length() - 1; i3++) {
			switch(str_where[i3]) {
				case '(': {if(!quo1 && !quo2) par++; break;}
				case ')': {if(par > 0 && !quo1 && !quo2) par--; break;}
				case '[': {if(!quo1 && !quo2) bra++; break;}
				case ']': {if(bra > 0 && !quo1 && !quo2) bra--; break;}
				case '\"': {if(!quo2) quo1 = !quo1; break;}
				case '\'': {if(!quo1) quo2 = !quo2; break;}
				case AND_CH: {
					if(str_where[i3 + 1] == AND_CH && par == 0 && bra == 0 && !quo1 && !quo2) {
						wheres.push_back(str_where.substr(i, i3 - i));
						remove_blank_ends(wheres[wheres.size() - 1]);
						i3++;
						i = i3 + 1;
					}
					break;
				}
				case SPACE_CH: {
					if(par == 0 && bra == 0 && !quo1 && !quo2) {
						size_t i2 = str_where.find(SPACE, i3 + 1);
						if(i2 != string::npos) i2 -= i3 + 1;
						if((and_str_len > 0 && i2 == and_str_len && equalsIgnoreCase(and_str, str_where.substr(i3 + 1, and_str_len))) || (i2 == AND_str_len && equalsIgnoreCase(AND_str, str_where.substr(i3 + 1, AND_str_len)))) {
							wheres.push_back(str_where.substr(i, i3 - i));
							remove_blank_ends(wheres[wheres.size() - 1]);
							i3 = i3 + i2 + 1;
							i = i3 + 1;

						}
					}
					break;
				}
				default: {}
			}
		}
		str_where = str_where.substr(i, str_where.length() - i);
		remove_blank_ends(str_where);
		if(!str_where.empty()) wheres.push_back(str_where);
		for(size_t i2 = 0; i2 < wheres.size(); i2++) {
			size_t index = wheres[i2].find_first_of("=<>!", 1);
			if(index != string::npos && (wheres[i2][index] != '!' || (index < wheres[i2].length() - 1 && wheres[i2][index + 1] == '='))) {
				string sname = wheres[i2].substr(0, index);
				if(index < wheres[i2].length() - 1 && wheres[i2][index] == '=' && (wheres[i2][index + 1] == '>' || wheres[i2][index + 1] == '<' || wheres[i2][index + 1] == '!')) {
					wheres[i2][index] = wheres[i2][index + 1];
					wheres[i2][index + 1] = '=';
				}
				remove_blank_ends(sname);
				if(!variableNameIsValid(sname)) {
					where_vars.push_back(NULL);
					repeat.push_back(false);
				} else {
					string svalue = wheres[i2].substr(index + 1, wheres[i2].length() - (index + 1));
					bool b_equals = !svalue.empty() && wheres[i2][index] != '=' && svalue[0] == '=';
					if(b_equals) svalue.erase(0, 1);
					remove_blank_ends(svalue);
					Variable *v = NULL;
					if(wheres[i2][index] == '=')	{
						v = new KnownVariable("\x14", sname, svalue);
					} else {
						wheres[i2] = wheres[i2].substr(index, wheres[i2].length() - 1);
						bool b = false;
						for(size_t i = 0; i < where_vars.size(); i++) {
							if(where_vars[i] && !where_vars[i]->isKnown() && where_vars[i]->name() == sname) {
								where_vars.push_back(where_vars[i]);
								repeat.push_back(true);
								b = true;
								break;
							}
						}
						if(!b) v = new UnknownVariable("\x14", sname);
					}
					if(v) {
						repeat.push_back(false);
						size_t l = sname.length() - 1;
						if(l > UFV_LENGTHS) {
							ufvl.insert(ufvl.begin(), (void*) v);
							ufvl_t.insert(ufvl_t.begin(), 'v');
							priv->ufvl_us.insert(priv->ufvl_us.begin(), 0);
							ufvl_i.insert(ufvl_i.begin(), 1);
						} else {
							ufv[3][l].insert(ufv[3][l].begin(), (void*) v);
							ufv_i[3][l].insert(ufv_i[3][l].begin(), 1);
							priv->ufv_us[3][l].insert(priv->ufv_us[3][l].begin(), 0);
							for(size_t i = 0; i < ufv[2][l].size(); i++) {
								const ExpressionName *ename = &((ExpressionItem*) ufv[2][l][i])->getName(ufv_i[2][l][i]);
								if(priv->ufv_us[2][l][i] == 0 && ((ename->case_sensitive && ename->name == sname) || (!ename->case_sensitive && equalsIgnoreCase(ename->name, sname)))) {
									ufv_i[2][l][i] += 10000;
									where_its.push_back(ufv_i[2][l].begin() + i);
									break;
								}
							}
						}
						where_vars.push_back(v);
					}
				}
			}
		}
	}

	if(!where_vars.empty()) {
		if(where_vars.size() > 1) {
			mwhere->setType(STRUCT_LOGICAL_AND);
		}
		for(size_t i = 0; i < where_vars.size(); i++) {
			if(where_vars[i]) {
				MathStructure *m = new MathStructure;
				ComparisonType ct = COMPARISON_EQUALS;
				if(where_vars[i]->isKnown()) {
					parse(m, ((KnownVariable*) where_vars[i])->expression(), po);
				} else {
					bool b_equals = wheres[i].size() > 1 && wheres[i][1] == '=';
					if(wheres[i][0] == '>') {
						if(b_equals) ct = COMPARISON_EQUALS_GREATER;
						else ct = COMPARISON_GREATER;
					} else if(wheres[i][0] == '<') {
						if(b_equals) ct = COMPARISON_EQUALS_LESS;
						else ct = COMPARISON_LESS;
					} else {
						ct = COMPARISON_NOT_EQUALS;
					}
					wheres[i].erase(0, b_equals ? 2 : 1);
					parse(m, wheres[i], po);
				}
				if(mwhere->isZero()) {
					mwhere->set(where_vars[i]);
					mwhere->transform_nocopy(STRUCT_COMPARISON, m);
					mwhere->setComparisonType(ct);
				} else {
					mwhere->addChild(where_vars[i]);
					mwhere->last().transform_nocopy(STRUCT_COMPARISON, m);
					mwhere->last().setComparisonType(ct);
				}
			}
		}
		for(size_t i = 0; i < where_vars.size(); i++) {
			if(!where_vars[i]) {
				if(mwhere->isZero()) {
					parse(mwhere, wheres[i], po);
				} else {
					MathStructure *m = new MathStructure;
					parse(m, wheres[i], po);
					mwhere->insertChild_nocopy(m, i + 1);
				}
			}
		}
	}

	parse(mstruct, str, po);

	if(!where_vars.empty()) {
		for(size_t i = where_vars.size() - 1; ; i--) {
			if(!repeat[i] && where_vars[i] && where_vars[i]->category() == "\x14") {
				size_t l = where_vars[i]->name().length() - 1;
				if(l > UFV_LENGTHS) {
					for(size_t i2 = 0; i2 < ufvl.size(); i2++) {
						if(ufvl_t[i2] == 'v' && ((ExpressionItem*) ufvl[i2])->name() == where_vars[i]->name() && ((ExpressionItem*) ufvl[i2])->category() == "\x14") {
							ufvl.erase(ufvl.begin() + i2);
							ufvl_t.erase(ufvl_t.begin() + i2);
							priv->ufvl_us.erase(priv->ufvl_us.begin() + i2);
							ufvl_i.erase(ufvl_i.begin() + i2);
							break;
						}
					}
				} else {
					for(size_t i2 = 0; i2 < ufv[3][i].size(); i2++) {
						if(((ExpressionItem*) ufv[3][l][i2])->name() == where_vars[i]->name() && ((ExpressionItem*) ufv[3][l][i2])->category() == "\x14") {
							ufv[3][l].erase(ufv[3][l].begin() + i2);
							ufv_i[3][l].erase(ufv_i[3][l].begin() + i2);
							priv->ufv_us[3][l].erase(priv->ufv_us[3][l].begin() + i2);
							break;
						}
					}
				}
				where_vars[i]->destroy();
			}
			if(i == 0) break;
		}
		for(size_t i = 0; i < where_its.size(); i++) {
			*where_its[i] -= 10000;
		}
	}
}

bool contains_variable_name(const MathStructure &m, Variable *v) {
	if(m.isVariable() && !m.variable()->isKnown() && m.variable()->hasName(v->name())) {
		return true;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_variable_name(m[i], v)) return true;
	}
	return false;
}
void replace_variable_name(MathStructure &m, Variable *v) {
	if(m.isVariable() && !m.variable()->isKnown() && m.variable()->hasName(v->name())) {
		m.set(v, true);
		return;
	}
	if(m.isVariable() && m.variable()->isKnown() && contains_variable_name(((KnownVariable*) m.variable())->get(), v)) {
		m.set(((KnownVariable*) m.variable())->get());
		replace_variable_name(m, v);
		return;
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_variable_name(m[i], v);
	}
}

MathStructure Calculator::calculate(string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {

	string str2, str_where;
	
	bool provided_to = false;

	// retrieve expression after " to " and remove "to ..." from expression
	if(make_to_division) separateToExpression(str, str2, eo, true);

	// retrieve expression after " where " (or "/.") and remove "to ..." from expression
	separateWhereExpression(str, str_where, eo);

	// handle to expression provided as argument
	Unit *u = NULL;
	if(to_struct) {
		// ignore if expression contains "to" expression
		if(str2.empty()) {
			if(to_struct->isSymbolic() && !to_struct->symbol().empty()) {
				// if to_struct is symbol, treat as "to" string
				str2 = to_struct->symbol();
				remove_blank_ends(str2);
			} else if(to_struct->isUnit()) {
				// if to_struct is unit, convert to this unit (later)
				u = to_struct->unit();
			}
			provided_to = true;
		}
		to_struct->setUndefined();
	}

	MathStructure mstruct;
	current_stage = MESSAGE_STAGE_PARSING;
	size_t n_messages = messages.size();

	vector<Variable*> where_vars;
	vector<vector<size_t>::iterator> where_its;

	if(!str_where.empty()) {
		parseSigns(str_where, false);
		remove_duplicate_blanks(str_where);
		int par = 0, bra = 0;
		bool quo1 = false, quo2 = false;
		vector<string> wheres;
		size_t i = 0;
		for(size_t i3 = 0; i3 < str_where.length() - 1; i3++) {
			switch(str_where[i3]) {
				case '(': {if(!quo1 && !quo2) par++; break;}
				case ')': {if(par > 0 && !quo1 && !quo2) par--; break;}
				case '[': {if(!quo1 && !quo2) bra++; break;}
				case ']': {if(bra > 0 && !quo1 && !quo2) bra--; break;}
				case '\"': {if(!quo2) quo1 = !quo1; break;}
				case '\'': {if(!quo1) quo2 = !quo2; break;}
				case AND_CH: {
					if(str_where[i3 + 1] == AND_CH && par == 0 && bra == 0 && !quo1 && !quo2) {
						wheres.push_back(str_where.substr(i, i3 - i));
						remove_blank_ends(wheres[wheres.size() - 1]);
						i3++;
						i = i3 + 1;
					}
					break;
				}
				case SPACE_CH: {
					if(par == 0 && bra == 0 && !quo1 && !quo2) {
						size_t i2 = str_where.find(SPACE, i3 + 1);
						if(i2 != string::npos) i2 -= i3 + 1;
						if((and_str_len > 0 && i2 == and_str_len && equalsIgnoreCase(and_str, str_where.substr(i3 + 1, and_str_len))) || (i2 == AND_str_len && equalsIgnoreCase(AND_str, str_where.substr(i3 + 1, AND_str_len)))) {
							wheres.push_back(str_where.substr(i, i3 - i));
							remove_blank_ends(wheres[wheres.size() - 1]);
							i3 = i3 + i2 + 1;
							i = i3 + 1;

						}
					}
					break;
				}
				default: {}
			}
		}
		str_where = str_where.substr(i, str_where.length() - i);
		remove_blank_ends(str_where);
		if(!str_where.empty()) wheres.push_back(str_where);
		str_where = "";
		for(size_t i2 = 0; i2 < wheres.size(); i2++) {
			size_t index = wheres[i2].find_first_of("=<>!", 1);
			if(index != string::npos && (wheres[i2][index] != '!' || (index < wheres[i2].length() - 1 && wheres[i2][index + 1] == '='))) {
				string sname = wheres[i2].substr(0, index);
				if(index < wheres[i2].length() - 1 && wheres[i2][index] == '=' && (wheres[i2][index + 1] == '>' || wheres[i2][index + 1] == '<' || wheres[i2][index + 1] == '!')) {
					wheres[i2][index] = wheres[i2][index + 1];
					wheres[i2][index + 1] = '=';
				}
				remove_blank_ends(sname);
				string svalue = wheres[i2].substr(index + 1, wheres[i2].length() - (index + 1));
				bool b_equals = !svalue.empty() && wheres[i2][index] != '=' && svalue[0] == '=';
				if(b_equals) svalue.erase(0, 1);
				remove_blank_ends(svalue);
				bool b_reversed = false;
				if(wheres[i2][index] != '=' && !variableNameIsValid(sname) && variableNameIsValid(svalue)) {
					string sbak = svalue;
					svalue = sname;
					sname = sbak;
					b_reversed = true;
				}
				if(!variableNameIsValid(sname)) {
					if(!str_where.empty()) str_where += LOGICAL_AND;
					str_where += wheres[i2];
				} else {
					Variable *v = NULL;
					if(wheres[i2][index] == '=') {
						v = new KnownVariable("\x14", sname, svalue);
					} else {
						MathStructure m;
						beginTemporaryStopMessages();
						parse(&m, svalue, eo.parse_options);
						bool b_evaled = false;
						if(!m.isNumber()) {
							m.eval(eo);
							if(!m.isNumber() && eo.approximation == APPROXIMATION_EXACT) {
								EvaluationOptions eo2 = eo;
								eo2.approximation = APPROXIMATION_APPROXIMATE;
								m.eval(eo2);
							}
							b_evaled = true;
						}
						if(!m.isNumber() && variableNameIsValid(svalue)) {
							endTemporaryStopMessages();
							beginTemporaryStopMessages();
							string sbak = svalue;
							svalue = sname;
							sname = sbak;
							b_reversed = true;
							b_evaled = false;
							parse(&m, svalue, eo.parse_options);
							if(!m.isNumber()) {
								m.eval(eo);
								if(!m.isNumber() && eo.approximation == APPROXIMATION_EXACT) {
									EvaluationOptions eo2 = eo;
									eo2.approximation = APPROXIMATION_APPROXIMATE;
									m.eval(eo2);
								}
								b_evaled = true;
							}
						}
						if(m.isNumber() && !m.number().hasImaginaryPart()) {
							ComparisonType ct;
							if((!b_reversed && wheres[i2][index] == '>') || (b_reversed && wheres[i2][index] == '<')) {
								if(b_equals) ct = COMPARISON_EQUALS_GREATER;
								else ct = COMPARISON_GREATER;
							} else if((!b_reversed && wheres[i2][index] == '<') || (b_reversed && wheres[i2][index] == '>')) {
								if(b_equals) ct = COMPARISON_EQUALS_LESS;
								else ct = COMPARISON_LESS;
							} else {
								ct = COMPARISON_NOT_EQUALS;
							}
							if(b_evaled && (ct == COMPARISON_LESS || ct == COMPARISON_GREATER) && m.number().isInterval() && m.number().precision(true) > PRECISION + 10) {
								if(ct == COMPARISON_LESS) m.number() = m.number().lowerEndPoint();
								else m.number() = m.number().upperEndPoint();
							}
							Assumptions *ass = NULL;
							for(size_t i = 0; i < where_vars.size(); i++) {
								if(!where_vars[i]->isKnown() && where_vars[i]->name() == sname) {
									ass = ((UnknownVariable*) where_vars[i])->assumptions();
									break;
								}
							}
							// can only handle not equals if value is zero
							if((ct != COMPARISON_NOT_EQUALS || (!ass && m.isZero()))) {
								if(ass) {
									bool b = false;
									// change existing assumptions
									if(ct == COMPARISON_EQUALS_GREATER) {
										if(!ass->min() || (*ass->min() < m.number())) {
											ass->setMin(&m.number()); ass->setIncludeEqualsMin(true);
											b = true;
										} else if(*ass->min() >= m.number()) {
											b = true;
										}
									} else if(ct == COMPARISON_EQUALS_LESS) {
										if(!ass->max() || (*ass->max() > m.number())) {
											ass->setMax(&m.number()); ass->setIncludeEqualsMax(true);
											b = true;
										} else if(*ass->max() <= m.number()) {
											b = true;
										}
									} else if(ct == COMPARISON_GREATER) {
										if(!ass->min() || (ass->includeEqualsMin() && *ass->min() <= m.number()) || (!ass->includeEqualsMin() && *ass->min() < m.number())) {
											ass->setMin(&m.number()); ass->setIncludeEqualsMin(false);
											b = true;
										} else if((ass->includeEqualsMin() && *ass->min() > m.number()) || (!ass->includeEqualsMin() && *ass->min() >= m.number())) {
											b = true;
										}
									} else if(ct == COMPARISON_LESS) {
										if(!ass->max() || (ass->includeEqualsMax() && *ass->max() >= m.number()) || (!ass->includeEqualsMax() && *ass->max() > m.number())) {
											ass->setMax(&m.number()); ass->setIncludeEqualsMax(false);
											b = true;
										} else if((ass->includeEqualsMax() && *ass->max() < m.number()) || (!ass->includeEqualsMax() && *ass->max() <= m.number())) {
											b = true;
										}
									}
									endTemporaryStopMessages(b);
									if(!b) {
										if(!str_where.empty()) str_where += LOGICAL_AND;
										str_where += wheres[i2];
									}
								} else {
									// create a new unknown variable and modify the assumptions
									v = new UnknownVariable("\x14", sname);
									ass = new Assumptions();
									if(m.isZero()) {
										if(ct == COMPARISON_EQUALS_GREATER) ass->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
										else if(ct == COMPARISON_EQUALS_LESS) ass->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
										else if(ct == COMPARISON_GREATER) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
										else if(ct == COMPARISON_LESS) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
										else if(ct == COMPARISON_NOT_EQUALS) ass->setSign(ASSUMPTION_SIGN_NONZERO);
									} else {
										if(ct == COMPARISON_EQUALS_GREATER) {ass->setMin(&m.number()); ass->setIncludeEqualsMin(true);}
										else if(ct == COMPARISON_EQUALS_LESS) {ass->setMax(&m.number()); ass->setIncludeEqualsMax(true);}
										else if(ct == COMPARISON_GREATER) {ass->setMin(&m.number()); ass->setIncludeEqualsMin(false);}
										else if(ct == COMPARISON_LESS) {ass->setMax(&m.number()); ass->setIncludeEqualsMax(false);}
									}
									((UnknownVariable*) v)->setAssumptions(ass);
									endTemporaryStopMessages(true);
								}
							}
						} else {
							endTemporaryStopMessages();
							if(!str_where.empty()) str_where += LOGICAL_AND;
							str_where += wheres[i2];
						}
					}
					if(v) {
						size_t l = sname.length() - 1;
						if(l > UFV_LENGTHS) {
							ufvl.insert(ufvl.begin(), (void*) v);
							ufvl_t.insert(ufvl_t.begin(), 'v');
							priv->ufvl_us.insert(priv->ufvl_us.begin(), 0);
							ufvl_i.insert(ufvl_i.begin(), 1);
						} else {
							ufv[3][l].insert(ufv[3][l].begin(), (void*) v);
							ufv_i[3][l].insert(ufv_i[3][l].begin(), 1);
							priv->ufv_us[3][l].insert(priv->ufv_us[3][l].begin(), 0);
							for(size_t i = 0; i < ufv[2][l].size(); i++) {
								const ExpressionName *ename = &((ExpressionItem*) ufv[2][l][i])->getName(ufv_i[2][l][i]);
								if(priv->ufv_us[2][l][i] == 0 && ((ename->case_sensitive && ename->name == sname) || (!ename->case_sensitive && equalsIgnoreCase(ename->name, sname)))) {
									ufv_i[2][l][i] += 10000;
									where_its.push_back(ufv_i[2][l].begin() + i);
									break;
								}
							}
						}
						where_vars.push_back(v);
					}
				}
			}
		}
	}

	if(!where_vars.empty()) {
		for(size_t i = 0; i < where_vars.size(); i++) {
			if(where_vars[i]->isKnown()) {
				MathStructure m;
				parse(&m, ((KnownVariable*) where_vars[i])->expression(), eo.parse_options);
				calculate_rand(m, eo);
				((KnownVariable*) where_vars[i])->set(m);
			}
		}
	}

	// perform expression parsing
	parse(&mstruct, str, eo.parse_options);

	if(parsed_struct) {
		// set parsed_struct to parsed expression with preserved formatting
		beginTemporaryStopMessages();
		ParseOptions po = eo.parse_options;
		po.preserve_format = true;
		parse(parsed_struct, str, po);
		endTemporaryStopMessages();
	}

	// handle "where" expression
	if(!str_where.empty()) {

		// parse "where" expression
		MathStructure where_struct;
		parse(&where_struct, str_where, eo.parse_options);

		current_stage = MESSAGE_STAGE_CALCULATION;

		// replace answer variables and functions in expression before performing any replacements from "where" expression
		calculate_ans(mstruct, eo);

		string str_test = str_where;
		remove_blanks(str_test);

		// check if "where" expression includes function replacements
		bool empty_func = str_test.find("()=") != string::npos;

		if(mstruct.isComparison() || (mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SOLVE && mstruct.size() >= 1 && mstruct[0].isComparison())) {
			beginTemporaryStopMessages();
			MathStructure mbak(mstruct);
			if(handle_where_expression(where_struct, mstruct, eo, where_vars, empty_func)) {
				endTemporaryStopMessages(true);
			} else {
				endTemporaryStopMessages();
				mstruct = mbak;
				// if where expression handling fails we can add the parsed "where" expression using logical and,
				// if the original expression is an equation
				if(mstruct.isComparison()) mstruct.transform(STRUCT_LOGICAL_AND, where_struct);
				else {mstruct[0].transform(STRUCT_LOGICAL_AND, where_struct); mstruct.childUpdated(1);}
			}
		} else {
			if(eo.approximation == APPROXIMATION_EXACT) {
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_TRY_EXACT;
				handle_where_expression(where_struct, mstruct, eo2, where_vars, empty_func);
			} else {
				handle_where_expression(where_struct, mstruct, eo, where_vars, empty_func);
			}
		}
	}

	if(!where_vars.empty()) {
		for(size_t i = where_vars.size() - 1; ; i--) {
			if(where_vars[i]->category() == "\x14") {
				size_t l = where_vars[i]->name().length() - 1;
				if(l > UFV_LENGTHS) {
					for(size_t i2 = 0; i2 < ufvl.size(); i2++) {
						if(ufvl_t[i2] == 'v' && ((ExpressionItem*) ufvl[i2])->name() == where_vars[i]->name() && ((ExpressionItem*) ufvl[i2])->category() == "\x14") {
							ufvl.erase(ufvl.begin() + i2);
							ufvl_t.erase(ufvl_t.begin() + i2);
							priv->ufvl_us.erase(priv->ufvl_us.begin() + i2);
							ufvl_i.erase(ufvl_i.begin() + i2);
							break;
						}
					}
				} else {
					for(size_t i2 = 0; i2 < ufv[3][i].size(); i2++) {
						if(((ExpressionItem*) ufv[3][l][i2])->name() == where_vars[i]->name() && ((ExpressionItem*) ufv[3][l][i2])->category() == "\x14") {
							ufv[3][l].erase(ufv[3][l].begin() + i2);
							ufv_i[3][l].erase(ufv_i[3][l].begin() + i2);
							priv->ufv_us[3][l].erase(priv->ufv_us[3][l].begin() + i2);
							break;
						}
					}
				}
			}
			if(i == 0) break;
		}
		for(size_t i = 0; i < where_its.size(); i++) {
			*where_its[i] -= 10000;
		}
		if(str_where.empty()) calculate_ans(mstruct, eo);
		for(size_t i = 0; i < where_vars.size(); i++) {
			if(where_vars[i]->category() != "\x14") continue;
			replace_variable_name(mstruct, where_vars[i]);
		}
	}

	current_stage = MESSAGE_STAGE_CALCULATION;

	// perform calculation
	mstruct.eval(eo);

	current_stage = MESSAGE_STAGE_UNSET;

	if(!aborted()) {
		// do unit conversion
		bool b_units = mstruct.containsType(STRUCT_UNIT, true);
		if(u) {
			// convert to unit provided in to_struct
			if(to_struct) to_struct->set(u);
			if(b_units) {
				current_stage = MESSAGE_STAGE_CONVERSION;
				mstruct.set(convert(mstruct, u, eo, false, false, true, parsed_struct));
				if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
			}
		} else if(!str2.empty()) {
			// conversion using "to" expression
			if(provided_to) {
				mstruct.set(convert(mstruct, str2, eo, to_struct, false, parsed_struct));
			} else {
				string str2b;
				while(true) {
					separateToExpression(str2, str2b, eo, true);
					if(to_struct && !to_struct->isUndefined()) {
						MathStructure mto;
						mto.setUndefined();
						mstruct.set(convert(mstruct, str2, eo, &mto, true, parsed_struct));
						if(!mto.isUndefined()) to_struct->multiply(mto, true);
					} else {
						mstruct.set(convert(mstruct, str2, eo, to_struct, true, parsed_struct));
					}
					if(str2b.empty()) break;
					str2 = str2b;
				}
			}
		} else if(b_units) {
			// do automatic conversion
			current_stage = MESSAGE_STAGE_CONVERSION;
			switch(eo.auto_post_conversion) {
				case POST_CONVERSION_OPTIMAL: {
					mstruct.set(convertToOptimalUnit(mstruct, eo, false));
					break;
				}
				case POST_CONVERSION_BASE: {
					mstruct.set(convertToBaseUnits(mstruct, eo));
					break;
				}
				case POST_CONVERSION_OPTIMAL_SI: {
					mstruct.set(convertToOptimalUnit(mstruct, eo, true));
					break;
				}
				default: {}
			}
			if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
		}
	}

	// clean up all new messages (removes "wide interval" warning if final value does not contains any wide interval)
	cleanMessages(mstruct, n_messages + 1);

	current_stage = MESSAGE_STAGE_UNSET;

	// replace variables generated from "where" expression
	for(size_t i = 0; i < where_vars.size(); i++) {
		if(!where_vars[i]->isKnown()) {
			Variable *v = getActiveVariable(where_vars[i]->name());
			if(v && !v->isKnown()) {
				mstruct.replace(where_vars[i], v);
			} else {
				string sym = where_vars[i]->name();
				if(sym.length() > 2 && (sym[0] == '\"' || sym[0] == '\'') && sym[sym.length() - 1] == sym[0]) {
					mstruct.replace(where_vars[i], MathStructure(sym.substr(1, sym.length() - 2), true));
				}
			}
		}
		where_vars[i]->destroy();
	}

	if(aborted()) replace_unregistered_variables(mstruct);

	return mstruct;

}
MathStructure Calculator::calculate(const MathStructure &mstruct_to_calculate, const EvaluationOptions &eo, string to_str) {

	remove_blank_ends(to_str);
	MathStructure mstruct(mstruct_to_calculate);
	current_stage = MESSAGE_STAGE_CALCULATION;
	size_t n_messages = messages.size();
	mstruct.eval(eo);

	current_stage = MESSAGE_STAGE_CONVERSION;
	if(!to_str.empty()) {
		mstruct.set(convert(mstruct, to_str, eo));
	} else {
		switch(eo.auto_post_conversion) {
			case POST_CONVERSION_OPTIMAL: {
				mstruct.set(convertToOptimalUnit(mstruct, eo, false));
				break;
			}
			case POST_CONVERSION_BASE: {
				mstruct.set(convertToBaseUnits(mstruct, eo));
				break;
			}
			case POST_CONVERSION_OPTIMAL_SI: {
				mstruct.set(convertToOptimalUnit(mstruct, eo, true));
				break;
			}
			default: {}
		}
		if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
	}

	cleanMessages(mstruct, n_messages + 1);

	current_stage = MESSAGE_STAGE_UNSET;
	return mstruct;
}

string Calculator::print(const MathStructure &mstruct, int msecs, const PrintOptions &po) {
	startControl(msecs);
	MathStructure mstruct2(mstruct);
	mstruct2.format(po);
	string print_result = mstruct2.print(po);
	stopControl();
	return print_result;
}
string Calculator::print(const MathStructure &mstruct, int msecs, const PrintOptions &po, bool format, int colorize, int tagtype) {
	startControl(msecs);
	MathStructure mstruct2(mstruct);
	mstruct2.format(po);
	string print_result = mstruct2.print(po, format, colorize, tagtype);
	stopControl();
	return print_result;
}
string Calculator::printMathStructureTimeOut(const MathStructure &mstruct, int msecs, const PrintOptions &po) {
	return print(mstruct, msecs, po);
}
int Calculator::testCondition(string expression) {
	MathStructure mstruct = calculate(expression);
	if(mstruct.isNumber()) {
		if(mstruct.number().isPositive()) {
			return 1;
		} else {
			return 0;
		}
	}
	return -1;
}

void Calculator::startPrintControl(int milli_timeout) {
	startControl(milli_timeout);
}
void Calculator::abortPrint() {
	abort();
}
bool Calculator::printingAborted() {
	return aborted();
}
string Calculator::printingAbortedMessage() const {
	return abortedMessage();
}
string Calculator::timedOutString() const {
	return _("timed out");
}
bool Calculator::printingControlled() const {
	return isControlled();
}
void Calculator::stopPrintControl() {
	stopControl();
}

void Calculator::startControl(int milli_timeout) {
	b_controlled = true;
	i_aborted = 0;
	i_timeout = milli_timeout;
	if(i_timeout > 0) {
#ifndef CLOCK_MONOTONIC
		gettimeofday(&t_end, NULL);
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		t_end.tv_sec = ts.tv_sec;
		t_end.tv_usec = ts.tv_nsec / 1000;
#endif
		long int usecs = t_end.tv_usec + (long int) milli_timeout * 1000;
		t_end.tv_usec = usecs % 1000000;
		t_end.tv_sec += usecs / 1000000;
	}
}
bool Calculator::aborted() {
	if(!b_controlled) return false;
	if(i_aborted > 0) return true;
	if(i_timeout > 0) {
#ifndef CLOCK_MONOTONIC
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if(tv.tv_sec > t_end.tv_sec || (tv.tv_sec == t_end.tv_sec && tv.tv_usec > t_end.tv_usec)) {
#else
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		if(tv.tv_sec > t_end.tv_sec || (tv.tv_sec == t_end.tv_sec && tv.tv_nsec / 1000 > t_end.tv_usec)) {
#endif
			i_aborted = 2;
			return true;
		}
	}
	return false;
}
string Calculator::abortedMessage() const {
	if(i_aborted == 2) return _("timed out");
	return _("aborted");
}
bool Calculator::isControlled() const {
	return b_controlled;
}
void Calculator::stopControl() {
	b_controlled = false;
	i_aborted = 0;
	i_timeout = 0;
}

