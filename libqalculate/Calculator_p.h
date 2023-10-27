/*
    Qalculate

    Copyright (C) 2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

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

#ifndef CALCULATOR_P_H
#define CALCULATOR_P_H

enum {
	PROC_RPN_ADD,
	PROC_RPN_SET,
	PROC_RPN_OPERATION_1,
	PROC_RPN_OPERATION_2,
	PROC_RPN_OPERATION_F,
	PROC_NO_COMMAND
};

class Calculator_p {
	public:
		unordered_map<size_t, MathStructure*> id_structs;
		unordered_map<size_t, bool> ids_p;
		unordered_map<size_t, size_t> ids_ref;
		std::vector<size_t> ufvl_us;
		std::vector<size_t> ufv_us[4][20];
		std::vector<size_t> freed_ids;
		size_t ids_i;
		Number custom_input_base, custom_output_base;
		long int custom_input_base_i;
		Unit *local_currency;
		int use_binary_prefixes;
		MathFunction *f_cis, *f_erfi, *f_fresnels, *f_fresnelc, *f_dot, *f_times, *f_rdivide, *f_power, *f_parallel, *f_vertcat, *f_horzcat, *f_secant, *f_newton;
		Unit *u_byn;
		Unit *u_kelvin, *u_rankine, *u_celsius, *u_fahrenheit;
		Unit *custom_angle_unit;
		unordered_map<int, MathFunction*> id_functions;
		unordered_map<int, Variable*> id_variables;
		unordered_map<int, Unit*> id_units;
		unordered_map<Unit*, MathStructure*> composite_unit_base;
		time_t exchange_rates_time2[1], exchange_rates_check_time2[1];
		TemperatureCalculationMode temperature_calculation;
		bool matlab_matrices;
		bool persistent_plot;
		bool concise_uncertainty_input;
		int exchange_rates_url3;
		long int fixed_denominator;
};

class CalculateThread : public Thread {
  protected:
	virtual void run();
};

bool is_not_number(char c, int base);

#endif
