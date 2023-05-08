/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MATHSTRUCTURE_SUPPORT_H
#define MATHSTRUCTURE_SUPPORT_H

#include "support.h"

#include "MathStructure.h"

#define SWAP_CHILDREN(i1, i2)		{MathStructure *swap_mstruct = v_subs[v_order[i1]]; v_subs[v_order[i1]] = v_subs[v_order[i2]]; v_subs[v_order[i2]] = swap_mstruct;}
#define CHILD_TO_FRONT(i)		v_order.insert(v_order.begin(), v_order[i]); v_order.erase(v_order.begin() + (i + 1));
#define SET_CHILD_MAP(i)		setToChild(i + 1, true);
#define SET_MAP(o)			set(o, true);
#define SET_MAP_NOCOPY(o)		set_nocopy(o, true);
#define MERGE_APPROX_AND_PREC(o)	if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define CHILD_UPDATED(i)		if(!b_approx && CHILD(i).isApproximate()) b_approx = true; if(CHILD(i).precision() > 0 && (i_precision < 1 || CHILD(i).precision() < i_precision)) i_precision = CHILD(i).precision();
#define CHILDREN_UPDATED		for(size_t child_i = 0; child_i < SIZE; child_i++) {if(!b_approx && CHILD(child_i).isApproximate()) b_approx = true; if(CHILD(child_i).precision() > 0 && (i_precision < 1 || CHILD(child_i).precision() < i_precision)) i_precision = CHILD(child_i).precision();}

#define APPEND(o)		v_order.push_back(v_subs.size()); v_subs.push_back(new MathStructure(o)); if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define APPEND_NEW(o)		{v_order.push_back(v_subs.size()); MathStructure *m_append_new = new MathStructure(o); v_subs.push_back(m_append_new); if(!b_approx && m_append_new->isApproximate())	b_approx = true; if(m_append_new->precision() > 0 && (i_precision < 1 || m_append_new->precision() < i_precision)) i_precision = m_append_new->precision();}
#define APPEND_COPY(o)		v_order.push_back(v_subs.size()); v_subs.push_back(new MathStructure(*(o))); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define APPEND_POINTER(o)	{MathStructure *m_append = o; v_order.push_back(v_subs.size()); v_subs.push_back(m_append); if(!b_approx && m_append->isApproximate()) b_approx = true; if(m_append->precision() > 0 && (i_precision < 1 || m_append->precision() < i_precision)) i_precision = m_append->precision();}
#define APPEND_REF(o)		{MathStructure *m_append = o; v_order.push_back(v_subs.size()); v_subs.push_back(m_append); m_append->ref(); if(!b_approx && m_append->isApproximate()) b_approx = true; if(m_append->precision() > 0 && (i_precision < 1 || m_append->precision() < i_precision)) i_precision = m_append->precision();}
#define PREPEND(o)		v_order.insert(v_order.begin(), v_subs.size()); v_subs.push_back(new MathStructure(o)); if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define PREPEND_REF(o)		{MathStructure *m_append = o; v_order.insert(v_order.begin(), v_subs.size()); v_subs.push_back(m_append); m_append->ref(); if(!b_approx && m_append->isApproximate()) b_approx = true; if(m_append->precision() > 0 && (i_precision < 1 || m_append->precision() < i_precision)) i_precision = m_append->precision();}
#define INSERT_REF(o, i)	{MathStructure *m_append = o; v_order.insert(v_order.begin() + i, v_subs.size()); v_subs.push_back(m_append); m_append->ref(); if(!b_approx && m_append->isApproximate()) b_approx = true; if(m_append->precision() > 0 && (i_precision < 1 || m_append->precision() < i_precision)) i_precision = m_append->precision();}
#define CLEAR			v_order.clear(); for(size_t i = 0; i < v_subs.size(); i++) {v_subs[i]->unref();} v_subs.clear();
//#define REDUCE(v_size)		for(size_t v_index = v_size; v_index < v_order.size(); v_index++) {v_subs[v_order[v_index]]->unref(); v_subs.erase(v_subs.begin() + v_order[v_index]);} v_order.resize(v_size);
#define REDUCE(v_size)          {std::vector<size_t> v_tmp; v_tmp.resize(SIZE, 0); for(size_t v_index = v_size; v_index < v_order.size(); v_index++) {v_subs[v_order[v_index]]->unref(); v_subs[v_order[v_index]] = NULL; v_tmp[v_order[v_index]] = 1;} v_order.resize(v_size); for(std::vector<MathStructure*>::iterator v_it = v_subs.begin(); v_it != v_subs.end();) {if(*v_it == NULL) v_it = v_subs.erase(v_it); else ++v_it;} size_t i_change = 0; for(size_t v_index = 0; v_index < v_tmp.size(); v_index++) {if(v_tmp[v_index] == 1) i_change++; v_tmp[v_index] = i_change;} for(size_t v_index = 0; v_index < v_order.size(); v_index++) v_order[v_index] -= v_tmp[v_index];}
#define CHILD(v_index)		(*v_subs[v_order[v_index]])
#define SIZE			v_order.size()
#define LAST			(*v_subs[v_order[v_order.size() - 1]])
#define ERASE(v_index)		v_subs[v_order[v_index]]->unref(); v_subs.erase(v_subs.begin() + v_order[v_index]); for(size_t v_index2 = 0; v_index2 < v_order.size(); v_index2++) {if(v_order[v_index2] > v_order[v_index]) v_order[v_index2]--;} v_order.erase(v_order.begin() + (v_index));

#define IS_REAL(o)		(o.isNumber() && o.number().isReal())
#define IS_RATIONAL(o)		(o.isNumber() && o.number().isRational())

#define IS_A_SYMBOL(o)		((o.isSymbolic() || o.isVariable() || o.isFunction()) && o.representsScalar())

#define POWER_CLEAN(o)		if(o[1].isOne()) {o.setToChild(1);} else if(o[1].isZero()) {o.set(1, 1, 0);}

#define VALID_ROOT(o)		(o.size() == 2 && o[1].isNumber() && o[1].number().isInteger() && o[1].number().isPositive())
#define THIS_VALID_ROOT		(SIZE == 2 && CHILD(1).isNumber() && CHILD(1).number().isInteger() && CHILD(1).number().isPositive())

#define FUNCTION_PROTECTED(evalops, id) (evalops.protected_function != NULL && evalops.protected_function == CALCULATOR->getFunctionById(id))

void printRecursive(const MathStructure &mstruct);

std::string format_and_print(const MathStructure &mstruct);

struct sym_desc {
	MathStructure sym;
	Number deg_a;
	Number deg_b;
	Number ldeg_a;
	Number ldeg_b;
	Number max_deg;
	size_t max_lcnops;
	bool operator<(const sym_desc &x) const;
};
typedef std::vector<sym_desc> sym_desc_vec;

bool polynomial_long_division(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar_pre, MathStructure &mquotient, MathStructure &mrem, const EvaluationOptions &eo, bool check_args = false, bool for_newtonraphson = false);
void integer_content(const MathStructure &mpoly, Number &icontent);
bool interpolate(const MathStructure &gamma, const Number &xi, const MathStructure &xvar, MathStructure &minterp, const EvaluationOptions &eo);
bool get_first_symbol(const MathStructure &mpoly, MathStructure &xvar);
bool divide_in_z(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo);
bool prem(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mrem, const EvaluationOptions &eo, bool check_args = true);
bool sr_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo);
void polynomial_smod(const MathStructure &mpoly, const Number &xi, MathStructure &msmod, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_smod = 0);
bool heur_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, const sym_desc_vec &sym_stats, size_t var_i);
void add_symbol(const MathStructure &mpoly, sym_desc_vec &v);
void collect_symbols(const MathStructure &mpoly, sym_desc_vec &v);
void add_symbol(const MathStructure &mpoly, std::vector<MathStructure> &v);
void collect_symbols(const MathStructure &mpoly, std::vector<MathStructure> &v);
void get_symbol_stats(const MathStructure &m1, const MathStructure &m2, sym_desc_vec &v);
bool sqrfree(MathStructure &mpoly, const EvaluationOptions &eo);
bool sqrfree(MathStructure &mpoly, const std::vector<MathStructure> &symbols, const EvaluationOptions &eo);
bool simplify_functions(MathStructure &mstruct, const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_var = m_undefined);
bool factorize_find_multiplier(const MathStructure &mstruct, MathStructure &mnew, MathStructure &factor_mstruct, bool only_units = false);
bool is_unit_multiexp(const MathStructure &mstruct);
bool has_approximate_relation_to_base(Unit *u, bool do_intervals = true);
bool contains_approximate_relation_to_base(const MathStructure &m, bool do_intervals = true);
bool contains_diff_for(const MathStructure &m, const MathStructure &x_var);
bool separate_unit_vars(MathStructure &m, const EvaluationOptions &eo, bool only_approximate, bool dry_run = false);
void lcm_of_coefficients_denominators(const MathStructure &e, Number &nlcm);
void multiply_lcm(const MathStructure &e, const Number &lcm, MathStructure &mmul, const EvaluationOptions &eo);
bool do_simplification(MathStructure &mstruct, const EvaluationOptions &eo, bool combine_divisions = true, bool only_gcd = false, bool combine_only = false, bool recursive = true, bool limit_size = false, int i_run = 1);
bool warn_about_assumed_not_value(const MathStructure &mstruct, const MathStructure &mvalue, const EvaluationOptions &eo);
bool warn_about_denominators_assumed_nonzero(const MathStructure &mstruct, const EvaluationOptions &eo);
bool warn_about_denominators_assumed_nonzero_or_positive(const MathStructure &mstruct, const MathStructure &mstruct2, const EvaluationOptions &eo);
bool warn_about_denominators_assumed_nonzero_llgg(const MathStructure &mstruct, const MathStructure &mstruct2, const MathStructure &mstruct3, const EvaluationOptions &eo);
bool is_differentiable(const MathStructure &m);
int test_comparisons(const MathStructure &msave, MathStructure &mthis, const MathStructure &x_var, const EvaluationOptions &eo, bool sub = false, int alt = 0);
bool replace_function(MathStructure &m, MathFunction *f1, MathFunction *f2, const EvaluationOptions &eo);
bool replace_intervals_f(MathStructure &mstruct);
bool replace_f_interval(MathStructure &mstruct, const EvaluationOptions &eo);
bool fix_intervals(MathStructure &mstruct, const EvaluationOptions &eo, bool *failed = NULL, long int min_precision = 2, bool function_middle = false);
bool set_uncertainty(MathStructure &mstruct, MathStructure &munc, const EvaluationOptions &eo = default_evaluation_options, bool do_eval = false);
bool create_interval(MathStructure &mstruct, const MathStructure &m1, const MathStructure &m2);
bool combine_powers(MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo, const EvaluationOptions &feo);
bool contains_angle_unit(const MathStructure &m, const ParseOptions &po);
bool has_predominately_negative_sign(const MathStructure &mstruct);
void negate_struct(MathStructure &mstruct);
bool test_eval(MathStructure &mtest, const EvaluationOptions &eo);
bool has_interval_unknowns(MathStructure &m);
bool flattenMultiplication(MathStructure &mstruct);
void idm1(const MathStructure &mnum, bool &bfrac, bool &bint);
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr);
int idm3(MathStructure &mnum, Number &nr, bool expand);
bool combination_factorize(MathStructure &mstruct);
bool replace_interval_unknowns(MathStructure &m, bool do_assumptions = false);
bool remove_rad_unit(MathStructure &m, const EvaluationOptions &eo, bool top = true);
int contains_ass_intval(const MathStructure &m);
int compare_check_incompability(MathStructure *mtest);
bool calculate_nondifferentiable_functions(MathStructure &m, const EvaluationOptions &eo, bool recursive = true, bool do_unformat = true, int i_type = 0);
bool function_differentiable(MathFunction *o_function);
bool montecarlo(const MathStructure &minteg, Number &nvalue, const MathStructure &x_var, const EvaluationOptions &eo, Number a, Number b, Number n);
bool romberg(const MathStructure &minteg, Number &nvalue, const MathStructure &x_var, const EvaluationOptions &eo, Number a, Number b, long int max_steps = -1, long int min_steps = 6, bool safety_measures = true);
bool sync_approximate_units(MathStructure &m, const EvaluationOptions &feo, std::vector<KnownVariable*> *vars = NULL, std::vector<MathStructure> *uncs = NULL, bool do_intervals = true);
void fix_to_struct(MathStructure &m);
int has_information_unit(const MathStructure &m, bool top = true);
bool calculate_userfunctions(MathStructure &m, const MathStructure &x_mstruct, const EvaluationOptions &eo, bool b_vector = false);
bool comparison_is_not_equal(ComparisonResult cr);
bool comparison_is_equal_or_less(ComparisonResult cr);
bool comparison_is_equal_or_greater(ComparisonResult cr);
bool comparison_might_be_equal(ComparisonResult cr);
Variable *find_interval_replace_var_comp(MathStructure &m, const EvaluationOptions &eo, Variable **v);

void generate_plotvector(const MathStructure &m, MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure &x_vector, MathStructure &y_vector, const EvaluationOptions &eo, bool adaptive = true);
void generate_plotvector(const MathStructure &m, MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure &x_vector, MathStructure &y_vector, const EvaluationOptions &eo);

void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1);
void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
bool transform_expression_for_equals_save(std::string&, const ParseOptions&);
bool expression_contains_save_function(const std::string&, const ParseOptions&, bool = false);
void replace_internal_operators(std::string &str);

#define HAS_DEFAULT_ANGLE_UNIT(x) (x != ANGLE_UNIT_NONE && (x != ANGLE_UNIT_CUSTOM || CALCULATOR->customAngleUnit()))
#define NO_DEFAULT_ANGLE_UNIT(x) (x == ANGLE_UNIT_NONE || (x == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()))
#define DEFAULT_RADIANS(x) (x == ANGLE_UNIT_RADIANS || x == ANGLE_UNIT_NONE || (x == ANGLE_UNIT_CUSTOM && (!CALCULATOR->customAngleUnit() || CALCULATOR->customAngleUnit() == CALCULATOR->getRadUnit())))

MathStructure angle_units_in_turn(const EvaluationOptions &eo, long int num = 1, long int den = 2, bool recip = false);
void convert_to_radians(const MathStructure &mpre, MathStructure &mstruct, const EvaluationOptions &eo);
void set_fraction_of_turn(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den);
void add_fraction_of_turn(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den, bool append = false);
void multiply_by_fraction_of_radian(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den);
Unit *default_angle_unit(const EvaluationOptions &eo, bool return_rad_if_none = false);

#endif

