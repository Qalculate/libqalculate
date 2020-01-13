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
#include "QalculateDateTime.h"

#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
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
			mstruct->set(CALCULATOR->calculate(CALCULATOR->expression_to_calculate, CALCULATOR->tmp_evaluationoptions, CALCULATOR->tmp_parsedstruct, CALCULATOR->tmp_tostruct, CALCULATOR->tmp_maketodivision));
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
	for(unordered_map<size_t, bool>::iterator it = priv->ids_p.begin(); it != priv->ids_p.end(); ++it) {
		if(!it->second) {
			priv->freed_ids.push_back(it->first);
			priv->id_structs.erase(it->first);
			priv->ids_p.erase(it);
		}
	}
}
bool Calculator::abort() {
	i_aborted = 1;
	if(!b_busy) return true;
	if(!calculate_thread->running) {
		b_busy = false;
	} else {
		// wait 5 seconds for clean abortation
		int msecs = 5000;
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
			if(tmp_rpn_mstruct) tmp_rpn_mstruct->unref();
			tmp_rpn_mstruct = NULL;

			// thread cancellation is not safe
			error(true, _("The calculation has been forcibly terminated. Please restart the application and report this as a bug."), NULL);

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
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {
						(*mstruct)[i - 1].multiply(getDegUnit());
						break;
					}
					case ANGLE_UNIT_GRADIANS: {
						(*mstruct)[i - 1].multiply(getGraUnit());
						break;
					}
					case ANGLE_UNIT_RADIANS: {
						(*mstruct)[i - 1].multiply(getRadUnit());
						break;
					}
					default: {}
				}
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
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {
						(*mstruct)[i - 1].multiply(getDegUnit());
						break;
					}
					case ANGLE_UNIT_GRADIANS: {
						(*mstruct)[i - 1].multiply(getGraUnit());
						break;
					}
					case ANGLE_UNIT_RADIANS: {
						(*mstruct)[i - 1].multiply(getRadUnit());
						break;
					}
					default: {}
				}
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

void fix_to_struct(MathStructure &m) {
	if(m.isPower() && m[0].isUnit()) {
		if(m[0].prefix() == NULL && m[0].unit()->referenceName() == "g") {
			m[0].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
		} else if(m[0].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m[0].setUnit(u);
		}
	} else if(m.isUnit()) {
		if(m.prefix() == NULL && m.unit()->referenceName() == "g") {
			m.setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
		} else if(m.unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m.setUnit(u);
		}
	} else {
		for(size_t i = 0; i < m.size();) {
			if(m[i].isUnit()) {
				if(m[i].prefix() == NULL && m[i].unit()->referenceName() == "g") {
					m[i].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
				} else if(m[i].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i].setUnit(u);
				}
				i++;
			} else if(m[i].isPower() && m[i][0].isUnit()) {
				if(m[i][0].prefix() == NULL && m[i][0].unit()->referenceName() == "g") {
					m[i][0].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
				} else if(m[i][0].unit() == CALCULATOR->getUnitById(UNIT_ID_EURO)) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i][0].setUnit(u);
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

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))
string Calculator::calculateAndPrint(string str, int msecs, const EvaluationOptions &eo, const PrintOptions &po) {

	if(msecs > 0) startControl(msecs);

	PrintOptions printops = po;
	EvaluationOptions evalops = eo;
	MathStructure mstruct;
	bool do_bases = false, do_factors = false, do_fraction = false, do_pfe = false, do_calendars = false, do_expand = false, do_binary_prefixes = false, complex_angle_form = false;

	// separate and handle string after "to"
	string from_str = str, to_str;
	Number base_save;
	if(printops.base == BASE_CUSTOM) base_save = customOutputBase();
	int save_bin = priv->use_binary_prefixes;
	if(separateToExpression(from_str, to_str, evalops, true)) {
		remove_duplicate_blanks(to_str);
		string to_str1, to_str2;
		size_t ispace = to_str.find_first_of(SPACES);
		if(ispace != string::npos) {
			to_str1 = to_str.substr(0, ispace);
			remove_blank_ends(to_str1);
			to_str2 = to_str.substr(ispace + 1);
			remove_blank_ends(to_str2);
		}
		if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", _("hexadecimal"))) {
			str = from_str;
			printops.base = BASE_HEXADECIMAL;
		} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
			str = from_str;
			printops.base = BASE_BINARY;
		} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", _("decimal"))) {
			str = from_str;
			printops.base = BASE_DECIMAL;
		} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
			str = from_str;
			printops.base = BASE_OCTAL;
		} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", _("duodecimal"))) {
			str = from_str;
			printops.base = BASE_DUODECIMAL;
		} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
			str = from_str;
			printops.base = BASE_ROMAN_NUMERALS;
		} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, _("bijective"))) {
			str = from_str;
			printops.base = BASE_BIJECTIVE_26;
		} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, _("sexagesimal"))) {
			str = from_str;
			printops.base = BASE_SEXAGESIMAL;
		} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
			str = from_str;
			printops.base = BASE_TIME;
		} else if(equalsIgnoreCase(to_str, "unicode")) {
			str = from_str;
			printops.base = BASE_UNICODE;
		} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
			str = from_str;
			printops.time_zone = TIME_ZONE_UTC;
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
			str = from_str;
		} else if(to_str == "CET") {
			printops.time_zone = TIME_ZONE_CUSTOM;
			printops.custom_time_zone = 60;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "cartesian", _("cartesian")) || str == "rect") {
			str = from_str;
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "exponential", _("exponential")) || to_str == "exp") {
			str = from_str;
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "polar", _("polar"))) {
			str = from_str;
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "angle", _("angle")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "phasor", _("phasor"))) {
			str = from_str;
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
			complex_angle_form = true;
		} else if(to_str == "cis") {
			str = from_str;
			evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "fraction", _("fraction"))) {
			str = from_str;
			do_fraction = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors")) || equalsIgnoreCase(to_str, "factor")) {
			str = from_str;
			evalops.structuring = STRUCTURING_FACTORIZE;
			do_factors = true;
		}  else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction"))) {
			str = from_str;
			do_pfe = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases"))) {
			do_bases = true;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", _("calendars"))) {
			do_calendars = true;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _("base"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_BASE;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", _("base"))) {
			str = from_str;
			if(to_str2 == "b26" || to_str2 == "B26") printops.base = BASE_BIJECTIVE_26;
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
					setCustomOutputBase(m.number());
				}
			}
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", _("mixed"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_NONE;
			evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
		} else {
			// ? in front of unit epxression is interpreted as a request for the optimal prefix.
			evalops.parse_options.units_enabled = true;
			if(to_str[0] == '?' || (to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'a' || to_str[0] == 'd'))) {
				printops.use_unit_prefixes = true;
				printops.use_prefixes_for_currencies = true;
				printops.use_prefixes_for_all_units = true;
				if(to_str[0] == 'a') printops.use_all_prefixes = true;
				else if(to_str[0] == 'd') priv->use_binary_prefixes = 0;
			} else if(to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'b') {
				// b? in front of unit epxression: use binary prefixes
				do_binary_prefixes = true;
			}
		}
		// expression after "to" is by default interpreted as unit epxression
	} else {
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
	}


	// perform calculation
	if(to_str.empty() || str == from_str) {
		mstruct = calculate(str, evalops);
	} else {
		// handle case where conversion to units requested, but original expression and result does not contains any unit
		MathStructure parsed_struct, to_struct;
		mstruct = calculate(str, evalops, &parsed_struct, &to_struct);
		if(to_struct.containsType(STRUCT_UNIT, true) && !mstruct.contains(STRUCT_UNIT, false, true, true) && !parsed_struct.containsType(STRUCT_UNIT, false, true, true)) {
			// convert "to"-expression to base units
			to_struct = CALCULATOR->convertToBaseUnits(to_struct);
			// remove non-units, set local currency and use kg instead of g
			fix_to_struct(to_struct);
			// recalculate
			if(!to_struct.isZero()) mstruct = calculate(mstruct, evalops, to_str);
		}
	}

	// handle "to factors", and "factor" or "expand" in front of the expression
	if(do_factors) {
		mstruct.integerFactorize();
	} else if(do_expand) {
		mstruct.expand(evalops, false);
	}

	// handle "to partial fraction"
	if(do_pfe) mstruct.expandPartialFractions(evalops);

	printops.allow_factorization = printops.allow_factorization || evalops.structuring == STRUCTURING_FACTORIZE || do_factors;

	if(do_calendars && mstruct.isDateTime()) {
		// handle "to calendars"
		str = "";
		bool b_fail;
		long int y, m, d;
#define PRINT_CALENDAR(x, c) if(!str.empty()) {str += "\n";} str += x; str += " "; b_fail = !dateToCalendar(*mstruct.datetime(), y, m, d, c); if(b_fail) {str += _("failed");} else {str += i2s(d); str += " "; str += monthName(m, c, true); str += " "; str += i2s(y);}
		PRINT_CALENDAR(string(_("Gregorian:")), CALENDAR_GREGORIAN);
		PRINT_CALENDAR(string(_("Hebrew:")), CALENDAR_HEBREW);
		PRINT_CALENDAR(string(_("Islamic:")), CALENDAR_ISLAMIC);
		PRINT_CALENDAR(string(_("Persian:")), CALENDAR_PERSIAN);
		PRINT_CALENDAR(string(_("Indian national:")), CALENDAR_INDIAN);
		PRINT_CALENDAR(string(_("Chinese:")), CALENDAR_CHINESE);
		long int cy, yc, st, br;
		chineseYearInfo(y, cy, yc, st, br);
		if(!b_fail) {str += " ("; str += chineseStemName(st); str += string(" "); str += chineseBranchName(br); str += ")";}
		PRINT_CALENDAR(string(_("Julian:")), CALENDAR_JULIAN);
		PRINT_CALENDAR(string(_("Revised julian:")), CALENDAR_MILANKOVIC);
		PRINT_CALENDAR(string(_("Coptic:")), CALENDAR_COPTIC);
		PRINT_CALENDAR(string(_("Ethiopian:")), CALENDAR_ETHIOPIAN);
		stopControl();
		return str;
	} else if(do_bases) {
		// handle "to bases"
		printops.base = BASE_BINARY;
		str = print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_OCTAL;
		str += print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_DECIMAL;
		str += print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_HEXADECIMAL;
		str += print(mstruct, 0, printops);
		stopControl();
		return str;
	} else if(do_fraction) {
		// handle "to fraction"
		if(mstruct.isNumber()) printops.number_fraction_format = FRACTION_COMBINED;
		else printops.number_fraction_format = FRACTION_FRACTIONAL;
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
	// do not display the default angle unit in trigonometric functions
	mstruct.removeDefaultAngleUnit(evalops);

	// format and print
	mstruct.format(printops);
	str = mstruct.print(printops);

	// "to angle": replace "cis" with angle symbol
	if(complex_angle_form) gsub(" cis ", "∠", str);

	if(msecs > 0) stopControl();

	// restore options
	if(printops.base == BASE_CUSTOM) setCustomOutputBase(base_save);
	priv->use_binary_prefixes = save_bin;

	return str;
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
	size_t i = str.length() - 1, i2 = i;
	int l = 2;
	while(i != 0) {
		i2 = str.rfind(_("to"), i - 1);
		i = str.rfind("to", i - 1);
		if(i2 != string::npos && (i == string::npos || i < i2)) {l = strlen(_("to")); i = i2;}
		else l = 2;
		if(i == string::npos) break;
		if(((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
	}
	return false;
}
bool Calculator::hasToExpression(const string &str, bool allow_empty_from, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	if(str.empty()) return false;
	size_t i = str.length() - 1, i2 = i;
	int l = 2;
	while(i != 0) {
		i2 = str.rfind(_("to"), i - 1);
		i = str.rfind("to", i - 1);
		if(i2 != string::npos && (i == string::npos || i < i2)) {l = strlen(_("to")); i = i2;}
		else l = 2;
		if(i == string::npos) break;
		if(((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
	}
	return false;
}
bool Calculator::separateToExpression(string &str, string &to_str, const EvaluationOptions &eo, bool keep_modifiers, bool allow_empty_from) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	to_str = "";
	if(str.empty()) return false;
	size_t i = str.length() - 1, i2 = i;
	int l = 2;
	while(i != 0) {
		i2 = str.rfind(_("to"), i - 1);
		i = str.rfind("to", i - 1);
		if(i2 != string::npos && (i == string::npos || i < i2)) {l = strlen(_("to")); i = i2;}
		else l = 2;
		if(i == string::npos) break;
		if(((i > 0 && is_in(SPACES, str[i - 1])) || (allow_empty_from && i == 0)) && i + l < str.length() && is_in(SPACES, str[i + l])) {
			to_str = str.substr(i + l , str.length() - i - l);
			if(to_str.empty()) return false;
			remove_blank_ends(to_str);
			if(!to_str.empty()) {
				if(to_str.rfind(SIGN_MINUS, 0) == 0) {
					to_str.replace(0, strlen(SIGN_MINUS), MINUS);
				}
				if(!keep_modifiers && (to_str[0] == '0' || to_str[0] == '?' || to_str[0] == '+' || to_str[0] == '-')) {
					to_str = to_str.substr(1, str.length() - 1);
					remove_blank_ends(to_str);
				} else if(!keep_modifiers && to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
					to_str = to_str.substr(2, str.length() - 2);
					remove_blank_ends(to_str);
				}
			}
			str = str.substr(0, i);
			return true;
		}
	}
	return false;
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
		else l = 2;
		if(i == string::npos) break;
		if(i > 0 && is_in(SPACES, str[i - 1]) && i + l < str.length() && is_in(SPACES, str[i + l])) return true;
	}
	if((i = str.rfind("/.", str.length() - 2)) != string::npos && eo.parse_options.base >= 2 && eo.parse_options.base <= 10 && (str[i + 2] < '0' || str[i + 2] > '9')) return true;
	return false;
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
		remove_blank_ends(to_str);
		str = str.substr(0, i);
		parseSigns(str);
		if(str.find("&&") == string::npos) {
			int par = 0;
			int bra = 0;
			for(size_t i = 0; i < str.length(); i++) {
				switch(str[i]) {
					case '(': {par++; break;}
					case ')': {if(par > 0) par--; break;}
					case '[': {bra++; break;}
					case ']': {if(bra > 0) bra--; break;}
					case COMMA_CH: {
						if(par == 0 && bra == 0) {
							str.replace(i, 1, LOGICAL_AND);
							i++;
						}
						break;
					}
					default: {}
				}
			}
		}
		return true;
	}
	return false;
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
bool handle_where_expression(MathStructure &m, MathStructure &mstruct, const EvaluationOptions &eo, vector<UnknownVariable*>& vars, vector<MathStructure>& varms, bool empty_func, bool do_eval = true) {
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
				if(m2.isComparison()) return handle_where_expression(m2, mstruct, eo, vars, varms, false, false);
			}
			if(m[0].isFunction() && m[1].isFunction() && (m[0].size() == 0 || (empty_func && m[0].function()->minargs() == 0)) && (m[1].size() == 0 || (empty_func && m[1].function()->minargs() == 0))) {
				// if left value is a function without any arguments, do function replacement
				if(!replace_function(mstruct, m[0].function(), m[1].function(), eo)) CALCULATOR->error(false, _("Original value (%s) was not found."), (m[0].function()->name() + "()").c_str(), NULL);
			} else {
				if(mstruct.countOccurrences(m[0]) > 1) {

					// make sure that only a single random value is used
					calculate_rand(m[1], eo);

					if(m[1].containsInterval(true, false, false, 0, true)) {
						// replace interval with variable if multiple occurrences if original value
						MathStructure mv(m[1]);
						replace_f_interval(mv, eo);
						replace_intervals_f(mv);
						if(!mstruct.replace(m[0], mv)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
					} else {
						if(!mstruct.replace(m[0], m[1])) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
					}
				} else {
					if(!mstruct.replace(m[0], m[1])) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
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
				for(size_t i = 0; i < varms.size(); i++) {
					if(varms[i] == m[0]) {
						ass = vars[0]->assumptions();
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
						varms.push_back(m[0]);
						MathStructure u_var(var);
						if(!mstruct.replace(m[0], u_var)) CALCULATOR->error(false, _("Original value (%s) was not found."), format_and_print(m[0]).c_str(), NULL);
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
			return handle_where_expression(m, mstruct, eo, vars, varms, false, false);
		}
	} else if(m.isLogicalAnd()) {
		// logical and (e.g. x=2&&y=3): perform multiple "where" replacments
		bool ret = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(!handle_where_expression(m[i], mstruct, eo, vars, varms, empty_func, do_eval)) ret = false;
		}
		return ret;
	}
	CALCULATOR->error(true, _("Unhandled \"where\" expression: %s"), format_and_print(m).c_str(), NULL);
	return false;
}
MathStructure Calculator::calculate(string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {

	string str2, str_where;

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
		}
		to_struct->setUndefined();
	}

	MathStructure mstruct;
	current_stage = MESSAGE_STAGE_PARSING;
	size_t n_messages = messages.size();

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
	vector<UnknownVariable*> vars;
	vector<MathStructure> varms;
	if(!str_where.empty()) {

		// parse "where" expression
		MathStructure where_struct;
		parse(&where_struct, str_where, eo.parse_options);

		current_stage = MESSAGE_STAGE_CALCULATION;

		// replace answer variables and functions in expression before performing any replacements from "where" epxression
		calculate_ans(mstruct, eo);

		string str_test = str_where;
		remove_blanks(str_test);

		// check if "where" expression includes function replacements
		bool empty_func = str_test.find("()=") != string::npos;

		if(mstruct.isComparison() || (mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SOLVE && mstruct.size() >= 1 && mstruct[0].isComparison())) {
			beginTemporaryStopMessages();
			MathStructure mbak(mstruct);
			if(handle_where_expression(where_struct, mstruct, eo, vars, varms, empty_func)) {
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
				handle_where_expression(where_struct, mstruct, eo2, vars, varms, empty_func);
			} else {
				handle_where_expression(where_struct, mstruct, eo, vars, varms, empty_func);
			}
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
				mstruct.set(convert(mstruct, u, eo, false, false));
				if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
			}
		} else if(!str2.empty()) {
			// conversion using "to" expression
			mstruct.set(convert(mstruct, str2, eo, to_struct));
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
	for(size_t i = 0; i < vars.size(); i++) {
		mstruct.replace(vars[i], varms[i]);
		vars[i]->destroy();
	}

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

