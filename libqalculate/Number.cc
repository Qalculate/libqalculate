/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Number.h"
#include "Calculator.h"
#include "Function.h"

#include <limits.h>
#include <sstream>
#include <string.h>
#include "util.h"

#define BIT_PRECISION ((long int) ((PRECISION) * 3.3219281) + 100)
#define PRECISION_TO_BITS(p) (((p) * 3.3219281) + 100)
#define BITS_TO_PRECISION(p) (::ceil(((p) - 100) / 3.3219281))

#define TO_BIT_PRECISION(p) ((::ceil((p) * 3.3219281)))
#define FROM_BIT_PRECISION(p) ((::floor((p) / 3.3219281)))

#define PRINT_MPFR(x, y) mpfr_out_str(stdout, y, 0, x, MPFR_RNDU); cout << endl;
#define PRINT_MPZ(x, y) mpz_out_str(stdout, y, x); cout << endl;

#define CREATE_INTERVAL (CALCULATOR ? CALCULATOR->usesIntervalArithmetic() : true)

gmp_randstate_t randstate;

Number nr_e;

int char2val(const char &c, const int &base) {
	if(c <= '9') return c - '0';
	if(base == 12 && c == 'X') return 10;
	else if(base == 12 && c == 'E') return 11;
	else return c - 'A' + 10;
}

void insert_thousands_separator(string &str, const PrintOptions &po) {
	if(po.digit_grouping != DIGIT_GROUPING_NONE && (po.digit_grouping != DIGIT_GROUPING_LOCALE || !CALCULATOR->local_digit_group_separator.empty())) {
		size_t i_deci = str.rfind(po.decimalpoint());
		size_t i;
		if(i_deci != string::npos) {
			i = i_deci;
			/*i += 5 + po.decimalpoint().length();
			while(i < str.length()) {
				str.insert(i, po.thousandsseparator());
				i += 5 + po.thousandsseparator().length();
			}
			i = i_deci;*/
		} else {
			i = str.length();
		}
		size_t group_size = 3, i_format = 0;
		if(po.digit_grouping == DIGIT_GROUPING_LOCALE && CALCULATOR->local_digit_group_format.size() > i_format) {
			if(CALCULATOR->local_digit_group_format[i_format] == CHAR_MAX) return;
			if(CALCULATOR->local_digit_group_format[i_format] > 0) group_size = CALCULATOR->local_digit_group_format[i_format];
		}
		if(po.digit_grouping == DIGIT_GROUPING_LOCALE || i > group_size + 1) {
			while(i > group_size) {
				i -= group_size;
				if(po.digit_grouping != DIGIT_GROUPING_LOCALE) {
#ifdef _WIN32
					str.insert(i, " ");
#else
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (" ", po.can_display_unicode_string_arg))) {
						str.insert(i, " ");
					} else {
						str.insert(i, " ");
					}
#endif
				} else {
					str.insert(i, CALCULATOR->local_digit_group_separator);
				}
				if(po.digit_grouping == DIGIT_GROUPING_LOCALE && CALCULATOR->local_digit_group_format.size() - 1 > i_format) {
					i_format++;
					if(CALCULATOR->local_digit_group_format[i_format] == CHAR_MAX) break;
					if(CALCULATOR->local_digit_group_format[i_format] > 0) group_size = CALCULATOR->local_digit_group_format[i_format];
				}
			}
		}
	}
}

string format_number_string(string cl_str, int base, BaseDisplay base_display, bool show_neg, bool format_base_two = true, const PrintOptions &po = default_print_options) {
	if(format_base_two && (base == 2 || (base == 16 && po.binary_bits >= 8)) && base_display != BASE_DISPLAY_NONE) {
		unsigned int bits = po.binary_bits;
		size_t l = cl_str.find(po.decimalpoint());
		if(l == string::npos) l = cl_str.length();
		if(bits == 0) {
			bits = l;
			if(bits % 4 != 0) bits += 4 - bits % 4;
		}
		if(base == 16) {
			bits /= 4;
		}
		if(l < bits) {
			string str;
			str.resize(bits - l, '0');
			cl_str = str + cl_str;
			l = bits;
		}
		if(base == 2 && base_display == BASE_DISPLAY_NORMAL) {
			for(int i = (int) l - 4; i > 0; i -= 4) {
				cl_str.insert(i, 1, ' ');
			}
		}
	}
	string str = "";
	if(show_neg) {
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) str += SIGN_MINUS;
		else str += '-';
	}
	if(base_display == BASE_DISPLAY_NORMAL) {
		if(base == 16) {
			str += "0x";
		} else if(base == 8) {
			str += "0";
		}
	} else if(base_display == BASE_DISPLAY_ALTERNATIVE) {
		if(base == 16) {
			str += "0x0";
		} else if(base == 8) {
			str += "0";
		} else if(base == 2) {
			str += "0b00";
		} 
	}
	if(base == BASE_DUODECIMAL) {
		for(size_t i = 0; i < cl_str.length(); i++) {
			if(cl_str[i] == 'A' || cl_str[i] == 'a' || cl_str[i] == 'X') {
				if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("↊", po.can_display_unicode_string_arg))) {cl_str.replace(i, 1, "↊"); i += strlen("↊") - 1;}
				else cl_str[i] = 'X';
			} else if(cl_str[i] == 'B' || cl_str[i] == 'b' || cl_str[i] == 'E') {
				if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("↋", po.can_display_unicode_string_arg))) {cl_str.replace(i, 1, "↋"); i += strlen("↋") - 1;}
				else cl_str[i] = 'E';
			}
		}
	} else if(base == BASE_DECIMAL) {
		insert_thousands_separator(cl_str, po);
	}
	str += cl_str;
	return str;
}

string printMPZ(mpz_ptr integ_pre, int base = 10, bool display_sign = true, bool lower_case = false) {
	int sign = mpz_sgn(integ_pre);
	if(base == BASE_ROMAN_NUMERALS) {
		if(sign != 0 && mpz_cmpabs_ui(integ_pre, 10000) == -1) {
			string str;
			int value = (int) mpz_get_si(integ_pre);
			if(value < 0) {
				value = -value;
				if(display_sign) {
					str += "-";
				}
			}
			int times = value / 1000;
			for(; times > 0; times--) {
				if(lower_case) str += "m";
				else str += "M";
			}
			value = value % 1000;
			times = value / 100;
			if(times == 9) {
				if(lower_case) str += "c";
				else str += "C";
				if(lower_case) str += "m";
				else str += "M";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "d";
				else str += "D";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "c";
				else str += "C";
				if(lower_case) str += "d";
				else str += "D";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "c";
				else str += "C";
			}
			value = value % 100;
			times = value / 10;
			if(times == 9) {
				if(lower_case) str += "x";
				else str += "X";
				if(lower_case) str += "c";
				else str += "C";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "l";
				else str += "L";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "x";
				else str += "X";
				if(lower_case) str += "l";
				else str += "L";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "x";
				else str += "X";
			}
			value = value % 10;
			times = value;
			if(times == 9) {
				if(lower_case) str += "i";
				else str += "I";
				if(lower_case) str += "x";
				else str += "X";
				times = 0;
			} else if(times >= 5) {
				if(lower_case) str += "v";
				else str += "V";
				times -= 5;
			} else if(times == 4) {
				times = 0;
				if(lower_case) str += "i";
				else str += "I";
				if(lower_case) str += "v";
				else str += "V";
			}
			for(; times > 0; times--) {
				if(lower_case) str += "i";
				else str += "I";
			}
			return str;
		} else if(sign != 0) {
			CALCULATOR->error(false, _("Cannot display numbers greater than 9999 or less than -9999 as roman numerals."), NULL);
		}
		base = 10;
	}
	
	string cl_str;
	
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	if(sign == -1) {
		mpz_neg(integ, integ);
		if(display_sign) cl_str += "-";
	}
	
	char *tmp = mpz_get_str(NULL, base, integ); 
	cl_str += tmp;
	void (*freefunc)(void *, size_t);
	mp_get_memory_functions (NULL, NULL, &freefunc);
	freefunc(tmp, strlen(tmp) + 1);
	
	if(base > 10) {
		if(lower_case) {
			for(size_t i = 0; i < cl_str.length(); i++) {
				if(cl_str[i] >= 'A' && cl_str[i] <= 'Z') {
					cl_str[i] += 32;
				}
			}
		} else {
			for(size_t i = 0; i < cl_str.length(); i++) {
				if(cl_str[i] >= 'a' && cl_str[i] <= 'z') {
					cl_str[i] -= 32;
				}
			}
		}
	}
	if(cl_str[cl_str.length() - 1] == '.') {
		cl_str.erase(cl_str.length() - 1, 1);
	}
	
	mpz_clear(integ);
	
	return cl_str;
}
string printMPZ(mpz_srcptr integ_pre, int base = 10, bool display_sign = true, bool lower_case = false) {
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	string str = printMPZ(integ, base, display_sign, lower_case);
	mpz_clear(integ);
	return str;
}

Number::Number() {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	clear();
}
Number::Number(string number, const ParseOptions &po) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(number, po);
}
Number::Number(long int numerator, long int denominator, long int exp_10) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(numerator, denominator, exp_10);
}
Number::Number(const Number &o) {
	b_imag = false;
	i_value = NULL;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_init(r_value);
	set(o);
}
Number::~Number() {
	mpq_clear(r_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	if(i_value) delete i_value;
}

void Number::set(string number, const ParseOptions &po) {

	size_t pm_index = number.find(SIGN_PLUSMINUS);
	if(pm_index == string::npos) pm_index = number.find("+/-");
	if(pm_index != string::npos) {
		ParseOptions po2 = po;
		po2.read_precision = DONT_READ_PRECISION;
		set(number.substr(0, pm_index), po2);
		number = number.substr(pm_index + (number[pm_index] == '+' ? strlen("+/-") : strlen(SIGN_PLUSMINUS)));
		if(!number.empty()) {
			Number pm_nr(number, po2);
			setUncertainty(pm_nr);
		}
		return;
	}

	if(po.base == BASE_ROMAN_NUMERALS) {
		remove_blanks(number);
		string number_bak = number;
		bool rev_c = (number.find("Ɔ") != string::npos);
		if(rev_c) gsub("Ɔ", ")", number);
		Number nr;
		Number cur;
		bool large = false;
		vector<Number> numbers;
		for(size_t i = 0; i < number.length(); i++) {
			switch(number[i]) {
				case 'I': {
					if(i > 0 && i == number.length() - 1 && number[i - 1] != 'i' && number[i - 1] != 'j' && number.find_first_of("jvxlcdm") != string::npos && number.find_first_of("IJVXLCDM") == i) {
						cur.set(2);
						CALCULATOR->error(false, _("Assuming the unusual practice of letting a last capital I mean 2 in a roman numeral."), NULL);
						break;
					}
				}
				case 'J': {}
				case 'i': {}
				case 'j': {
					cur.set(1);
					break;
				}
				case 'V': {}
				case 'v': {
					cur.set(5);
					break;
				}
				case 'X': {}
				case 'x': {
					cur.set(10);
					break;
				}
				case 'L': {}
				case 'l': {
					cur.set(50);
					break;
				}
				case 'C': {}
				case 'c': {
					if(rev_c) {
						size_t i2 = number.find_first_not_of("Cc", i);
						if(i2 != string::npos && number[i2] == '|' && i2 + (i2 - i) < number.length() && number[i2 + (i2 - i)] == ')') {
							bool b = true;
							for(size_t i3 = i2 + 1; i3 < i2 + (i2 - i); i3++) {
								if(number[i3] != ')') {b = false; break;}
							}
							if(b) {
								cur.set(1, 1, i2 - i + 2);
								i = i2 + (i2 - i);
								if(i + 1 < number.length() && number[i + 1] == ')') {
									i2 = number.find_first_not_of(")", i + 2);
									if(i2 == string::npos) {
										cur += Number(5, 1, number.length() - i);
										i = number.length() - 1;
									} else {
										cur += Number(5, 1, i2 - i);
										i = i2 - 1;
									}
								}
								break;
							}
						}
					}
					cur.set(100);
					break;
				}
				case 'D': {}
				case 'd': {
					cur.set(500);
					break;
				}
				case 'M': {}
				case 'm': {
					cur.set(1000);
					break;
				}
				case '|': {
					if(large) {
						cur.clear();
						large = false;
						break;
					} else if(i + 1 < number.length() && number[i + 1] == ')') {
						size_t i2 = number.find_first_not_of(")", i + 2);
						if(i2 == string::npos) {
							cur.set(5, 1, number.length() - i - 1);
							i = number.length() - 1;
						} else {
							cur.set(5, 1, i2 - i);
							i = i2 - 1;
						}
						break;
					} else if(i + 2 < number.length() && number.find("|", i + 2) != string::npos) {
						cur.clear();
						large = true;
						break;
					}
					CALCULATOR->error(true, _("Error in roman numerals: %s."), number_bak.c_str(), NULL);
					break;
				}
				case '(': {
					if(!rev_c) {
						size_t i2 = number.find_first_not_of("(", i);
						if(i2 != string::npos && number[i2] == '|' && i2 + (i2 - i) < number.length() && number[i2 + (i2 - i)] == ')') {
							bool b = true;
							for(size_t i3 = i2 + 1; i3 < i2 + (i2 - i); i3++) {
								if(number[i3] != ')') {b = false; break;}
							}
							if(b) {
								cur.set(1, 1, i2 - i + 2);
								i = i2 + (i2 - i);
								if(i + 1 < number.length() && number[i + 1] == ')') {
									i2 = number.find_first_not_of(")", i + 2);
									if(i2 == string::npos) {
										cur += Number(5, 1, number.length() - i);
										i = number.length() - 1;
									} else {
										cur += Number(5, 1, i2 - i);
										i = i2 - 1;
									}
								}
								break;
							}
						}
						CALCULATOR->error(true, _("Error in roman numerals: %s."), number_bak.c_str(), NULL);
						break;
					}
				}
				case ')': {
					CALCULATOR->error(true, _("Error in roman numerals: %s."), number_bak.c_str(), NULL);
					break;
				}
				default: {
					cur.clear();
					CALCULATOR->error(true, _("Unknown roman numeral: %c."), number[i], NULL);
				}
			}
			if(!cur.isZero()) {
				if(large) {
					cur.multiply(100000L);
				}
				numbers.resize(numbers.size() + 1);
				numbers[numbers.size() - 1].set(cur);
			}
		}
		vector<Number> values;
		values.resize(numbers.size());
		bool error = false;
		int rep = 1;
		for(size_t i = 0; i < numbers.size(); i++) {
			if(i == 0 || numbers[i].isLessThanOrEqualTo(numbers[i - 1])) {
				nr.add(numbers[i]);
				if(i > 0 && numbers[i].equals(numbers[i - 1])) {
					rep++;
					if(rep > 3 && numbers[i].isLessThan(1000)) {
						error = true;
					} else if(rep > 1 && (numbers[i].equals(5) || numbers[i].equals(50) || numbers[i].equals(500))) {
						error = true;
					}
				} else {
					rep = 1;
				}
			} else {	
				numbers[i - 1].multiply(10);
				if(numbers[i - 1].isLessThan(numbers[i])) {
					error = true;
				}
				numbers[i - 1].divide(10);
				for(int i2 = i - 2; ; i2--) {
					if(i2 < 0) {
						nr.negate();
						nr.add(numbers[i]);
						break;
					} else if(numbers[i2].isGreaterThan(numbers[i2 + 1])) {
						Number nr2(nr);
						nr2.subtract(values[i2]);
						nr.subtract(nr2);
						nr.subtract(nr2);
						nr.add(numbers[i]);
						if(numbers[i2].isLessThan(numbers[i])) {
							error = true;
						}
						break;
					}
					error = true;
				}
			}
			values[i].set(nr);
		}
		if(error && number.find('|') == string::npos) {
			PrintOptions pro;
			pro.base = BASE_ROMAN_NUMERALS;
			CALCULATOR->error(false, _("Errors in roman numerals: \"%s\". Interpreted as %s, which should be written as %s."), number.c_str(), nr.print().c_str(), nr.print(pro).c_str(), NULL);
		}
		values.clear();
		numbers.clear();
		set(nr);
		return;
	}
	long int i_unc = 0;
	mpz_t num, den;
	mpq_t unc;
	mpz_init(num);
	mpz_init_set_ui(den, 1);
	int base = po.base;
	remove_blank_ends(number);
	if(base == 16 && number.length() >= 2 && number[0] == '0' && (number[1] == 'x' || number[1] == 'X')) {
		number = number.substr(2, number.length() - 2);
	} else if(base == 8 && number.length() >= 2 && number[0] == '0' && (number[1] == 'o' || number[1] == 'O')) {
		number = number.substr(2, number.length() - 2);
	} else if(base == 8 && number.length() > 1 && number[0] == '0' && number[1] != '.') {
		number.erase(number.begin());
	} else if(base == 2 && number.length() >= 2 && number[0] == '0' && (number[1] == 'b' || number[1] == 'B')) {
		number = number.substr(2, number.length() - 2);
	}
	bool b_twos = (po.twos_complement && base == 2 && number.length() > 1 && number[0] == '1') || (po.hexadecimal_twos_complement && base == 16 && number.length() > 0 && (number[0] == '8' || number[0] == '9' || (number[0] >= 'a' && number[0] <= 'f') || (number[0] >= 'A' && number[0] <= 'F')));
	if(base > 36) base = 36;
	if(base < 0) base = 10;
	long int readprec = 0;
	bool numbers_started = false, minus = false, in_decimals = false, b_cplx = false, had_nonzero = false;
	for(size_t index = 0; index < number.size(); index++) {
		if(number[index] >= '0' && ((base >= 10 && number[index] <= '9') || (base < 10 && number[index] < '0' + base))) {
			mpz_mul_si(num, num, base);
			if(number[index] != (b_twos ? '0' + (base - 1) : '0')) {
				mpz_add_ui(num, num, b_twos ? (unsigned long int) (base - 1) - (number[index] - '0') : (unsigned long int) number[index] - '0');
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			readprec++;
			numbers_started = true;
		} else if(base == BASE_DUODECIMAL && (number[index] == 'X' || number[index] == 'E' || number[index] == 'x' || number[index] == 'e')) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (number[index] == 'E' || number[index] == 'e') ? 11L : 10L);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'a' && number[index] < 'a' + base - 10) {
			mpz_mul_si(num, num, base);
			if(!b_twos || (number[index] != 'a' + (base - 11))) {
				mpz_add_ui(num, num, b_twos ? (unsigned long int) (base - 1) - (number[index] - 'a' + 10) : (unsigned long int) number[index] - 'a' + 10);
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'A' && number[index] < 'A' + base - 10) {
			mpz_mul_si(num, num, base);
			if(!b_twos || (number[index] != 'A' + (base - 11))) {
				mpz_add_ui(num, num, b_twos ? (unsigned long int) (base - 1) - (number[index] - 'A' + 10) : (unsigned long int) number[index] - 'A' + 10);
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			readprec++;
			numbers_started = true;
		} else if((number[index] == 'E' || number[index] == 'e') && base <= 10) {
			index++;
			numbers_started = false;
			bool exp_minus = false;
			unsigned long int exp = 0;
			unsigned long int max_exp = ULONG_MAX / 10;
			while(index < number.size()) {
				if(number[index] >= '0' && number[index] <= '9') {
					if(exp > max_exp) {
						CALCULATOR->error(true, _("Too large exponent."), NULL);
					} else {
						exp = exp * 10;
						exp = exp + number[index] - '0';
						numbers_started = true;
					}
				} else if(!numbers_started && number[index] == '-') {
					exp_minus = !exp_minus;
				}
				index++;
			}
			if(exp_minus) {
				mpz_t e_den;
				mpz_init(e_den);
				mpz_ui_pow_ui(e_den, 10, exp);
				mpz_mul(den, den, e_den);
				if(i_unc > 0) mpz_mul(mpq_denref(unc), mpq_denref(unc), e_den);
				mpz_clear(e_den);
			} else {
				mpz_t e_num;
				mpz_init(e_num);
				mpz_ui_pow_ui(e_num, 10, exp);
				mpz_mul(num, num, e_num);
				if(i_unc > 0) mpz_mul(mpq_numref(unc), mpq_numref(unc), e_num);
				mpz_clear(e_num);
			}
			break;
		} else if(number[index] == '.') {
			in_decimals = true;
		} else if(number[index] == ':') {
			if(in_decimals) {
				CALCULATOR->error(true, _("\':\' in decimal number ignored (decimal point detected)."), NULL);
			} else {
				size_t index_colon = index;
				Number divisor(1, 1);
				Number num_temp;
				clear();
				i_precision = -1;				
				index = 0;				
				while(index_colon < number.size()) {
					num_temp.set(number.substr(index, index_colon - index), po);
					if(!num_temp.isZero()) {
						num_temp.divide(divisor);						
						add(num_temp);
					}
					index = index_colon + 1;
					index_colon = number.find(":", index);
					divisor.multiply(Number(60, 1));
				}
				num_temp.set(number.substr(index), po);
				if(!num_temp.isZero()) {
					num_temp.divide(divisor);
					add(num_temp);
				}
				mpz_clears(num, den, NULL);
				return;
			}
		} else if(!numbers_started && number[index] == '-') {
			minus = !minus;
		} else if(number[index] == 'i') {
			b_cplx = true;
		} else if(base == 10 && number[index] == '(' && index <= number.length() - 2) {
			size_t par_i = number.find(')', index + 1);
			if(par_i == string::npos) {
				i_unc = s2i(number.substr(index + 1));
				index = number.length() - 1;
			} else if(par_i > index + 1) {
				i_unc = s2i(number.substr(index + 1, par_i - index - 1));
				index = par_i;
			}
			if(i_unc > 0) {
				mpq_init(unc);
				mpz_set(mpq_denref(unc), den);
				mpz_set_ui(mpq_numref(unc), i_unc);
			}
		} else if(number[index] != ' ') {
			CALCULATOR->error(true, _("Character \'%c\' was ignored in the number \"%s\" with base %s."), number[index], number.c_str(), i2s(base).c_str(), NULL);
		}
	}
	if(b_twos) {
		mpz_add_ui(num, num, 1);
		minus = !minus;
	}
	clear();
	if(i_unc <= 0 && (po.read_precision == ALWAYS_READ_PRECISION || (in_decimals && po.read_precision == READ_PRECISION_WHEN_DECIMALS))) {
		mpz_mul_si(num, num, 2);
		mpz_mul_si(den, den, 2);
		
		mpq_t rv1, rv2;
		mpq_inits(rv1, rv2, NULL);
		
		mpz_add_ui(num, num, 1);
		if(minus) mpz_neg(mpq_numref(rv1), num);
		else mpz_set(mpq_numref(rv1), num);
		mpz_set(mpq_denref(rv1), den);
		mpq_canonicalize(rv1);
		
		mpz_sub_ui(num, num, 2);
		if(minus) mpz_neg(mpq_numref(rv2), num);
		else mpz_set(mpq_numref(rv2), num);
		mpz_set(mpq_denref(rv2), den);
		mpq_canonicalize(rv2);

		mpfr_init2(fu_value, BIT_PRECISION);
		mpfr_init2(fl_value, BIT_PRECISION);
		mpfr_clear_flags();
			
		mpfr_set_q(fu_value, minus ? rv2 : rv1, MPFR_RNDD);
		mpfr_set_q(fl_value, minus ? rv1 : rv2, MPFR_RNDU);
		
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);

		n_type = NUMBER_TYPE_FLOAT;
		
		b_approx = true;
		
		testErrors(2);
		
		if(b_cplx) {
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			i_value->set(*this, false, true);
			clearReal();
		}
		
		mpq_clears(rv1, rv2, NULL);
	} else {
		if(minus) mpz_neg(num, num);
		if(b_cplx) {
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			i_value->setInternal(num, den, false, true);
			mpq_canonicalize(i_value->internalRational());
		} else {
			mpz_set(mpq_numref(r_value), num);
			mpz_set(mpq_denref(r_value), den);
			mpq_canonicalize(r_value);
		}
		if(i_unc > 0) {
			Number nr_unc;
			mpq_canonicalize(unc);
			nr_unc.setInternal(unc);
			setUncertainty(nr_unc);
			mpq_clear(unc);
		} else if(po.read_precision == ALWAYS_READ_PRECISION || (in_decimals && po.read_precision == READ_PRECISION_WHEN_DECIMALS)) {
			if(base != 10) {
				Number precmax(10);
				precmax.raise(readprec);
				precmax--;
				precmax.log(base);
				precmax.floor();
				readprec = precmax.intValue();
			}
			if(b_cplx) i_value->setPrecision(readprec);
			setPrecision(readprec);
		}
	}
	mpz_clears(num, den, NULL);
}
void Number::set(long int numerator, long int denominator, long int exp_10, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	if(denominator < 0) {
		denominator = -denominator;
		numerator = -numerator;
	}
	mpq_set_si(r_value, numerator, denominator == 0 ? 1 : denominator);
	mpq_canonicalize(r_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(exp_10 != 0) {
		exp10(exp_10);
	}
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setFloat(long double d_value) {
	b_approx = true;
	if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
	if(CREATE_INTERVAL) {
		mpfr_set_ld(fu_value, d_value, MPFR_RNDU);
		mpfr_set_ld(fl_value, d_value, MPFR_RNDD);
	} else {
		mpfr_set_ld(fl_value, d_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	}
	n_type = NUMBER_TYPE_FLOAT;
	mpq_set_ui(r_value, 0, 1);
	if(i_value) i_value->clear();
}
bool Number::setInterval(const Number &nr_lower, const Number &nr_upper, bool keep_precision) {

	Number nr_l(nr_lower), nr_u(nr_upper);
	
	if(nr_l == nr_u) {
		set(nr_l, true);
		setPrecisionAndApproximateFrom(nr_u);
		return true;
	}
	
	if(!nr_l.setToFloatingPoint() || !nr_u.setToFloatingPoint()) return false;
	
	clear(keep_precision);

	mpfr_init2(fu_value, BIT_PRECISION);
	mpfr_init2(fl_value, BIT_PRECISION);
	
	mpfr_clear_flags();

	if(mpfr_cmp(nr_l.internalUpperFloat(), nr_u.internalUpperFloat()) > 0) mpfr_set(fu_value, nr_l.internalUpperFloat(), MPFR_RNDU);
	else mpfr_set(fu_value, nr_u.internalUpperFloat(), MPFR_RNDU);
	if(mpfr_cmp(nr_l.internalLowerFloat(), nr_u.internalLowerFloat()) > 0) mpfr_set(fl_value, nr_u.internalLowerFloat(), MPFR_RNDD);
	else mpfr_set(fl_value, nr_l.internalLowerFloat(), MPFR_RNDD);
	
	setPrecisionAndApproximateFrom(nr_l);
	setPrecisionAndApproximateFrom(nr_u);
	
	if(!b_imag && (nr_l.hasImaginaryPart() || nr_u.hasImaginaryPart())) {
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->setInterval(nr_l.imaginaryPart(), nr_u.imaginaryPart(), keep_precision);
		setPrecisionAndApproximateFrom(*i_value);
	}
	
	b_approx = true;
	
	n_type = NUMBER_TYPE_FLOAT;
	
	return true;

}

void Number::setInternal(mpz_srcptr mpz_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set_z(r_value, mpz_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpz_t &mpz_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set_z(r_value, mpz_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpq_t &mpq_value, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpq_set(r_value, mpq_value);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpz_t &mpz_num, const mpz_t &mpz_den, bool keep_precision, bool keep_imag) {
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	mpz_set(mpq_numref(r_value), mpz_num);
	mpz_set(mpq_denref(r_value), mpz_den);
	if(n_type == NUMBER_TYPE_FLOAT) mpfr_clears(fu_value, fl_value, NULL);
	n_type = NUMBER_TYPE_RATIONAL;
	if(!keep_imag && i_value) i_value->clear();
	else if(i_value) setPrecisionAndApproximateFrom(*i_value);
}
void Number::setInternal(const mpfr_t &mpfr_value, bool merge_precision, bool keep_imag) {
	b_approx = true;
	if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
	if(CREATE_INTERVAL) {
		mpfr_set(fu_value, mpfr_value, MPFR_RNDU);
		mpfr_set(fl_value, mpfr_value, MPFR_RNDD);
	} else {
		mpfr_set(fl_value, mpfr_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	}
	n_type = NUMBER_TYPE_FLOAT;
	mpq_set_ui(r_value, 0, 1);
	if(!keep_imag && i_value) i_value->clear();
}

void Number::setImaginaryPart(const Number &o) {
	if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
	i_value->set(o, false, true);
	setPrecisionAndApproximateFrom(*i_value);
}
void Number::setImaginaryPart(long int numerator, long int denominator, long int exp_10) {
	if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
	i_value->set(numerator, denominator, exp_10);
}
void Number::set(const Number &o, bool merge_precision, bool keep_imag) {
	mpq_set(r_value, o.internalRational());
	if(o.internalType() == NUMBER_TYPE_FLOAT) {
		if(n_type != NUMBER_TYPE_FLOAT) {mpfr_init2(fu_value, BIT_PRECISION); mpfr_init2(fl_value, BIT_PRECISION);}
		if(CREATE_INTERVAL || o.isInterval()) {
			mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
			mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
		} else {
			mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		}
	}
	n_type = o.internalType();
	if(!merge_precision) {
		i_precision = -1;
		b_approx = false;
	}
	if(o.isApproximate()) b_approx = true;
	if(i_precision < 0 || o.precision() < i_precision) i_precision = o.precision();
	if(!keep_imag && !b_imag) {
		if(o.hasImaginaryPart()) {
			setImaginaryPart(*o.internalImaginary());
		} else if(i_value) {
			i_value->clear();
		}
	}
}
void Number::setPlusInfinity(bool keep_precision, bool keep_imag) {
	if(keep_imag) clearReal();
	else clear(keep_precision);
	n_type = NUMBER_TYPE_PLUS_INFINITY;
}
void Number::setMinusInfinity(bool keep_precision, bool keep_imag) {
	if(keep_imag) clearReal();
	else clear(keep_precision);
	n_type = NUMBER_TYPE_MINUS_INFINITY;
}

void Number::clear(bool keep_precision) {
	if(i_value) i_value->clear();
	if(!keep_precision) {
		b_approx = false;
		i_precision = -1;
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear(fl_value);
		mpfr_clear(fu_value);
	}
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, 0, 1);
}
void Number::clearReal() {
	if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear(fl_value);
		mpfr_clear(fu_value);
	}
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, 0, 1);
}
void Number::clearImaginary() {
	if(i_value) i_value->clear();
}

const mpq_t &Number::internalRational() const {
	return r_value;
}
const mpfr_t &Number::internalUpperFloat() const {
	return fu_value;
}
const mpfr_t &Number::internalLowerFloat() const {
	return fl_value;
}
mpq_t &Number::internalRational() {
	return r_value;
}
mpfr_t &Number::internalUpperFloat() {
	return fu_value;
}
mpfr_t &Number::internalLowerFloat() {
	return fl_value;
}
Number *Number::internalImaginary() const {
	return i_value;
}
void Number::markAsImaginaryPart(bool is_imag) {
	b_imag = is_imag;
}
const NumberType &Number::internalType() const {
	return n_type;
}
bool Number::setToFloatingPoint() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION);
		mpfr_init2(fl_value, BIT_PRECISION);
		
		mpfr_clear_flags();

		if(n_type == NUMBER_TYPE_RATIONAL) {
			if(!CREATE_INTERVAL) {
				mpfr_set_q(fl_value, r_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_set_q(fu_value, r_value, MPFR_RNDU);
				mpfr_set_q(fl_value, r_value, MPFR_RNDD);
			}
			if(!testFloatResult(true, 1, false)) {
				mpfr_clears(fu_value, fl_value, NULL);
				return false;
			}
		} else if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
			mpfr_set_inf(fl_value, 1);
			mpfr_set_inf(fu_value, 1);
		} else if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
			mpfr_set_inf(fl_value, -1);
			mpfr_set_inf(fu_value, -1);
		} else {
			mpfr_clears(fu_value, fl_value, NULL);
			return false;
		}
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	}
	return true;
}
void Number::precisionToInterval() {
	if(hasImaginaryPart()) i_value->precisionToInterval();
	if(i_precision >= 0 && !isInfinite(true) && !isInterval()) {
		if(!setToFloatingPoint()) return;
		mpfr_clear_flags();
		mpfr_t f_log;
		mpfr_init2(f_log, mpfr_get_prec(fl_value));
		mpfr_abs(f_log, fu_value, MPFR_RNDN);
		mpfr_log10(f_log, f_log, MPFR_RNDN);
		mpfr_floor(f_log, f_log);
		mpfr_sub_ui(f_log, f_log, i_precision, MPFR_RNDN);
		mpfr_ui_pow(f_log, 10, f_log, MPFR_RNDD);
		mpfr_div_ui(f_log, f_log, 2, MPFR_RNDD);
		mpfr_sub(fl_value, fl_value, f_log, MPFR_RNDU);
		mpfr_add(fu_value, fu_value, f_log, MPFR_RNDD);
		mpfr_clear(f_log);
		testErrors(2);
		i_precision = -1;
	}
}
bool Number::intervalToPrecision(long int min_precision) {
	if(n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value)) {
		if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) return false;
		mpfr_clear_flags();
		mpfr_t f_diff, f_mid;
		mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
		mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
		mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
		mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
		mpfr_mul_ui(f_diff, f_diff, 2, MPFR_RNDN);
		mpfr_div(f_diff, f_mid, f_diff, MPFR_RNDN);
		mpfr_abs(f_diff, f_diff, MPFR_RNDN);
		if(mpfr_zero_p(f_diff)) {mpfr_clears(f_diff, f_mid, NULL); return false;}
		mpfr_log10(f_diff, f_diff, MPFR_RNDN);
		long int i_prec = mpfr_get_si(f_diff, MPFR_RNDD) + 1;
		if(i_prec < min_precision || testErrors(0)) {mpfr_clears(f_diff, f_mid, NULL); return false;}
		if(i_value && !i_value->intervalToPrecision()) {mpfr_clears(f_diff, f_mid, NULL); return false;}
		if(i_precision < 0 || i_prec < i_precision) i_precision = i_prec;
		mpfr_set(fl_value, f_mid, MPFR_RNDN);
		mpfr_set(fu_value, f_mid, MPFR_RNDN);
		mpfr_clears(f_diff, f_mid, NULL);
		b_approx = true;
	} else if(i_value && !i_value->intervalToPrecision()) return false;
	return true;
}
void Number::intervalToMidValue() {
	if(n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value)) {
		if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
			if(mpfr_inf_p(fl_value) && mpfr_inf_p(fu_value) && mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) clearReal();
			else if(mpfr_inf_p(fl_value)) mpfr_set(fu_value, fl_value, MPFR_RNDN);
			else mpfr_set(fl_value, fu_value, MPFR_RNDN);
		} else {
			mpfr_clear_flags();
			mpfr_sub(fu_value, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(fu_value, fu_value, 2, MPFR_RNDN);
			mpfr_add(fl_value, fl_value, fu_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
			if(!testFloatResult()) clearReal();
		}
	}
	if(i_value) i_value->intervalToMidValue();
}
void Number::splitInterval(unsigned int nr_of_parts, vector<Number> &v) const {
	if(n_type == NUMBER_TYPE_FLOAT && isReal()) {
		if(nr_of_parts == 2) {
			mpfr_t f_mid;
			mpfr_init2(f_mid, mpfr_get_prec(fl_value));
			mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(f_mid, f_mid, 2, MPFR_RNDN);
			mpfr_add(f_mid, f_mid, fl_value, MPFR_RNDN);
			v.push_back(*this);
			mpfr_set(v.back().internalUpperFloat(), f_mid, MPFR_RNDU);
			v.push_back(*this);
			//mpfr_nextabove(f_mid);
			mpfr_set(v.back().internalLowerFloat(), f_mid, MPFR_RNDD);
		} else {
			mpfr_t value_diff, lower_value, upper_value, value_add;
			mpfr_inits2(mpfr_get_prec(fl_value), value_diff, lower_value, upper_value, value_add, NULL);
			mpfr_sub(value_diff, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(value_diff, value_diff, nr_of_parts, MPFR_RNDN);
			mpfr_set(lower_value, fl_value, MPFR_RNDD);
			for(unsigned int i = 1; i <= nr_of_parts; i++) {
				mpfr_mul_ui(value_add, value_diff, i, MPFR_RNDU);
				mpfr_add(upper_value, fl_value, value_add, MPFR_RNDU);
				if(mpfr_cmp(upper_value, fu_value) > 0) mpfr_set(upper_value, fu_value, MPFR_RNDU);
				v.push_back(*this);
				mpfr_set(v.back().internalLowerFloat(), lower_value, MPFR_RNDD);
				mpfr_set(v.back().internalUpperFloat(), upper_value, MPFR_RNDU);
				mpfr_set(lower_value, upper_value, MPFR_RNDD);
			}
		}
	}
}
bool Number::getCentralInteger(Number &nr_int, bool *b_multiple, vector<Number> *v) const {
	if(!isInterval() || !isReal()) {
		if(b_multiple) *b_multiple = false;
		return false;
	}
	mpfr_t fintl, fintu;
	mpfr_init2(fintl, mpfr_get_prec(fl_value));
	mpfr_init2(fintu, mpfr_get_prec(fu_value));
	mpfr_floor(fintu, fu_value);
	mpfr_ceil(fintl, fl_value);
	int cmp = mpfr_cmp(fintl, fintu);
	if(cmp == 0) {
		mpz_t z_int;
		mpz_init(z_int);
		mpfr_get_z(z_int, fl_value, MPFR_RNDN);
		nr_int.setInternal(z_int);
		if(b_multiple) *b_multiple = false;
		if(v) {
			mpfr_t f_prec;
			mpfr_init2(f_prec, mpfr_get_prec(fl_value));
			mpfr_ui_pow_ui(f_prec, 10, PRECISION + 10, MPFR_RNDN);
			mpfr_div(f_prec, fintl, f_prec, MPFR_RNDN);
			if(mpfr_cmp(fintl, fl_value) > 0) {
				mpfr_sub(fintl, fintl, f_prec, MPFR_RNDD);
				v->push_back(*this);
				mpfr_set(v->back().internalUpperFloat(), fintl, MPFR_RNDD);
			}
			if(mpfr_cmp(fintu, fu_value) < 0) {
				mpfr_add(fintu, fintu, f_prec, MPFR_RNDU);
				v->push_back(*this);
				mpfr_set(v->back().internalLowerFloat(), fintu, MPFR_RNDU);
			}
		}
		return true;
	} else if(cmp > 0) {
		if(b_multiple) *b_multiple = false;
	} else {
		if(b_multiple) *b_multiple = true;
	}
	return false;
}
bool Number::mergeInterval(const Number &o, bool set_to_overlap) {
	if(equals(o)) return true;
	if(!isReal() || !o.isReal()) return false;
	if(isRational()) {
		mpfr_init2(fu_value, BIT_PRECISION);
		mpfr_init2(fl_value, BIT_PRECISION);
		
		mpfr_clear_flags();

		if(o.isRational()) {
			if(set_to_overlap) {mpfr_clears(fu_value, fl_value, NULL); return false;}
			if(mpq_cmp(r_value, o.internalRational()) > 0) {
				mpfr_set_q(fl_value, o.internalRational(), MPFR_RNDD);
				mpfr_set_q(fu_value, r_value, MPFR_RNDU);
			} else {
				mpfr_set_q(fu_value, o.internalRational(), MPFR_RNDU);
				mpfr_set_q(fl_value, r_value, MPFR_RNDD);
			}
		} else {
			if(mpfr_cmp_q(o.internalUpperFloat(), r_value) < 0) {
				if(set_to_overlap) {mpfr_clears(fu_value, fl_value, NULL); return false;}
				mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
				mpfr_set_q(fu_value, r_value, MPFR_RNDU);
			} else if(mpfr_cmp_q(o.internalLowerFloat(), r_value) > 0) {
				if(set_to_overlap) {mpfr_clears(fu_value, fl_value, NULL); return false;}
				mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
				mpfr_set_q(fl_value, r_value, MPFR_RNDD);
			} else {
				if(set_to_overlap) {
					mpfr_clears(fu_value, fl_value, NULL);
					setPrecisionAndApproximateFrom(o);
					return true;
				}
				mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
				mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
			}
		}
		
		if(!testFloatResult(true, 1, false)) {
			mpfr_clears(fu_value, fl_value, NULL);
			return false;
		}
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	} else if(o.isRational()) {
		if(mpfr_cmp_q(fu_value, o.internalRational()) < 0) {
			if(set_to_overlap) return false;
			mpfr_set_q(fu_value, o.internalRational(), MPFR_RNDU);
		} else if(mpfr_cmp_q(fl_value, o.internalRational()) > 0) {
			if(set_to_overlap) return false;
			mpfr_set_q(fl_value, o.internalRational(), MPFR_RNDD);
		} else {
			if(set_to_overlap) {
				set(o, true);
				return true;
			}
		}
	} else if(set_to_overlap) {
		if(mpfr_cmp(fl_value, o.internalUpperFloat()) > 0 || mpfr_cmp(fu_value, o.internalLowerFloat()) < 0) {
			return false;
		} else {
			if(mpfr_cmp(fl_value, o.internalLowerFloat()) < 0) mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
			if(mpfr_cmp(fu_value, o.internalUpperFloat()) > 0) mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
		}
	} else {
		if(mpfr_cmp(fl_value, o.internalLowerFloat()) > 0) mpfr_set(fl_value, o.internalLowerFloat(), MPFR_RNDD);
		if(mpfr_cmp(fu_value, o.internalUpperFloat()) < 0) mpfr_set(fu_value, o.internalUpperFloat(), MPFR_RNDU);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}

void Number::setUncertainty(const Number &o, bool to_precision) {
	if(o.hasImaginaryPart()) {
		if(!i_value) i_value = new Number();
		i_value->setUncertainty(o.imaginaryPart(), to_precision);
		setPrecisionAndApproximateFrom(*i_value);
		if(o.hasRealPart()) setUncertainty(o.realPart(), to_precision);
		return;
	}
	if(o.isInfinite()) {
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_inits2(BIT_PRECISION, fl_value, fu_value, NULL);
		}
		mpfr_set_inf(fl_value, -1);
		mpfr_set_inf(fu_value, 1);
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
		return;
	}
	if(isInfinite()) return;
	b_approx = true;
	if(to_precision && !isInterval()) {
		Number nr(*this);
		nr.divide(o);
		nr.abs();
		nr.divide(2);
		nr.log(10);
		nr.floor();
		long int i_prec = nr.lintValue();
		if(i_prec > 0) {
			if(i_precision < 0 || i_prec < i_precision) i_precision = i_prec;
			return;
		}
	}
	if(o.isNegative()) {
		Number o_abs(o);
		o_abs.negate();
		setUncertainty(o_abs, to_precision);
		return;
	}
	mpfr_clear_flags();
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpfr_inits2(BIT_PRECISION, fl_value, fu_value, NULL);
		if(o.isRational()) {
			mpq_sub(r_value, r_value, o.internalRational());
			mpfr_set_q(fl_value, r_value, MPFR_RNDD);
			mpq_add(r_value, r_value, o.internalRational());
			mpq_add(r_value, r_value, o.internalRational());
			mpfr_set_q(fu_value, r_value, MPFR_RNDU);
		} else {
			mpfr_sub_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
			mpfr_neg(fl_value, fl_value, MPFR_RNDD);
			mpfr_add_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
		}
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	} else if(o.isRational()) {
		mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
		mpfr_add_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
	} else {
		mpfr_sub(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
		mpfr_add(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
	}
	testErrors(2);
}
void Number::setRelativeUncertainty(const Number &o, bool to_precision) {
	Number nr(*this);
	nr.multiply(o);
	setUncertainty(nr, to_precision);
}
Number Number::uncertainty() const {
	if(!isInterval()) return Number();
	Number nr;
	if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
		nr.setPlusInfinity();
	} else {
		mpfr_clear_flags();
		mpfr_t f_mid;
		mpfr_init2(f_mid, BIT_PRECISION);
		mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDU);
		mpfr_div_ui(f_mid, f_mid, 2, MPFR_RNDU);
		nr.setInternal(f_mid);
		mpfr_clear(f_mid);
		nr.testFloatResult();
	}
	if(i_value) nr.setImaginaryPart(i_value->uncertainty());
	return nr;
}
Number Number::relativeUncertainty() const {
	if(!isInterval()) return Number();
	if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
		Number nr;
		nr.setPlusInfinity();
		return nr;
	}
	mpfr_clear_flags();
	mpfr_t f_mid, f_diff;
	mpfr_inits2(BIT_PRECISION, f_mid, f_diff, NULL);
	mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDU);
	mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDU);
	mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
	mpfr_abs(f_mid, f_mid, MPFR_RNDN);
	mpfr_div(f_mid, f_diff, f_mid, MPFR_RNDN);
	Number nr;
	nr.setInternal(f_mid);
	mpfr_clears(f_mid, f_diff, NULL);
	nr.testFloatResult();
	return nr;
}

double Number::floatValue() const {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		return mpq_get_d(r_value);
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_get_d(fu_value, MPFR_RNDN) / 2.0 + mpfr_get_d(fl_value, MPFR_RNDN) / 2.0;
	} 
	return 0.0;
}
int Number::intValue(bool *overflow) const {
	if(includesInfinity()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_sint_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return INT_MIN;
			return INT_MAX;	
		}
		return (int) mpz_get_si(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.intValue(overflow);
	}
}
unsigned int Number::uintValue(bool *overflow) const {
	if(includesInfinity()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_uint_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return 0;
			return UINT_MAX;	
		}
		return (unsigned int) mpz_get_ui(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.uintValue(overflow);
	}
}
long int Number::lintValue(bool *overflow) const {
	if(includesInfinity()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_slong_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return LONG_MIN;
			return LONG_MAX;	
		}
		return mpz_get_si(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.lintValue(overflow);
	}
}
long long int Number::llintValue() const {
	if(includesInfinity()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		long long result = 0;
		mpz_export(&result, 0, -1, sizeof result, 0, 0, mpq_numref(r_value));
		if(mpq_sgn(r_value) < 0) return -result;
		return result;
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.llintValue();
	}
}
unsigned long int Number::ulintValue(bool *overflow) const {
	if(includesInfinity()) return 0;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_fits_ulong_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return 0;
			return ULONG_MAX;
		}
		return mpz_get_ui(mpq_numref(r_value));
	} else {
		Number nr;
		nr.set(*this, false, true);
		nr.round();
		return nr.ulintValue(overflow);
	}
}

bool Number::isApproximate() const {
	return b_approx;
}
bool Number::isFloatingPoint() const {
	return (n_type == NUMBER_TYPE_FLOAT);
}
bool Number::isInterval(bool ignore_imag) const {
	return (n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value)) || (!ignore_imag && i_value && i_value->isInterval());
}
bool Number::imaginaryPartIsInterval() const {
	return i_value && i_value->isInterval();
}
void Number::setApproximate(bool is_approximate) {
	if(is_approximate != isApproximate()) {
		if(is_approximate) {
			//i_precision = PRECISION;
			b_approx = true;
		} else {
			i_precision = -1;
			b_approx = false;
		}
	}
}

int Number::precision(int calculate_from_interval) const {
	if(calculate_from_interval < 0) {
		int iv_prec = precision(1);
		if(i_precision < 0 || iv_prec < i_precision) return iv_prec;
	} else if(calculate_from_interval > 0) {
		if(n_type == NUMBER_TYPE_FLOAT && !mpfr_equal_p(fl_value, fu_value)) {
			mpfr_clear_flags();
			mpfr_t f_diff, f_mid;
			mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
			mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
			mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
			mpfr_mul_ui(f_diff, f_diff, 2, MPFR_RNDN);
			mpfr_div(f_diff, f_mid, f_diff, MPFR_RNDN);
			mpfr_abs(f_diff, f_diff, MPFR_RNDN);
			if(!mpfr_zero_p(f_diff)) mpfr_log10(f_diff, f_diff, MPFR_RNDN);
			int i_prec = -1;
			if(mpfr_sgn(f_diff) <= 0 || testErrors(0)) {
				i_prec = 0;
			} else if(mpfr_fits_sint_p(f_diff, MPFR_RNDU)) {
				i_prec = mpfr_get_si(f_diff, MPFR_RNDD) + 1;
			}
			if(i_value && i_prec != 0) {
				int imag_prec = i_value->precision(1);
				if(imag_prec >= 0 && (i_prec < 0 || imag_prec < i_prec)) i_prec = imag_prec;
			}
			mpfr_clears(f_diff, f_mid, NULL);
			return i_prec;
			
		} else if(i_value) return i_value->precision(1);
		return -1;
	}
	return i_precision;
}
void Number::setPrecision(int prec) {
	i_precision = prec;
	if(i_precision >= 0) b_approx = true;
}

bool Number::isUndefined() const {
	return false;
}
bool Number::isInfinite(bool ignore_imag) const {
	return n_type >= NUMBER_TYPE_PLUS_INFINITY && (ignore_imag || !i_value || i_value->isZero());
}
bool Number::isPlusInfinity(bool ignore_imag) const {
	return n_type == NUMBER_TYPE_PLUS_INFINITY && (ignore_imag || !i_value || i_value->isZero());
}
bool Number::isMinusInfinity(bool ignore_imag) const {
	return n_type == NUMBER_TYPE_MINUS_INFINITY && (ignore_imag || !i_value || i_value->isZero());
}
bool Number::includesInfinity(bool ignore_imag) const {
	return n_type >= NUMBER_TYPE_PLUS_INFINITY || (n_type == NUMBER_TYPE_FLOAT && (mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value))) || (!ignore_imag && i_value && i_value->includesInfinity());
}
bool Number::includesPlusInfinity() const {
	return n_type == NUMBER_TYPE_PLUS_INFINITY || (n_type == NUMBER_TYPE_FLOAT && (mpfr_inf_p(fu_value) && mpfr_sgn(fu_value) >= 0));
}
bool Number::includesMinusInfinity() const {
	return n_type == NUMBER_TYPE_MINUS_INFINITY || (n_type == NUMBER_TYPE_FLOAT && (mpfr_inf_p(fl_value) && mpfr_sgn(fl_value) < 0));
}

Number Number::realPart() const {
	Number real_part;
	real_part.set(*this, true, true);
	return real_part;
}
Number Number::imaginaryPart() const {
	if(!i_value) return Number();
	return *i_value;
}
Number Number::lowerEndPoint(bool include_imag) const {
	if(i_value && !i_value->isZero()) {
		if(include_imag) {
			if(!isInterval(false)) return *this;
			Number nr;
			if(isInterval(true)) nr.setInternal(fl_value);
			else nr.set(realPart());
			nr.setImaginaryPart(i_value->lowerEndPoint());
			nr.setPrecisionAndApproximateFrom(*this);
			return nr;
		}
		if(!isInterval()) return realPart();
	} else if(!isInterval()) return *this;
	Number nr;
	nr.setInternal(fl_value);
	nr.setPrecisionAndApproximateFrom(*this);
	return nr;
}
Number Number::upperEndPoint(bool include_imag) const {
	if(i_value && !i_value->isZero()) {
		if(include_imag) {
			if(!isInterval(false)) return *this;
			Number nr;
			if(isInterval(true)) nr.setInternal(fu_value);
			else nr.set(realPart());
			nr.setImaginaryPart(i_value->upperEndPoint());
			nr.setPrecisionAndApproximateFrom(*this);
			return nr;
		}
		if(!isInterval()) return realPart();
	} else if(!isInterval()) return *this;
	Number nr;
	nr.setInternal(fu_value);
	nr.setPrecisionAndApproximateFrom(*this);
	return nr;
}
Number Number::numerator() const {
	Number num;
	num.setInternal(mpq_numref(r_value));
	return num;
}
Number Number::denominator() const {
	Number den;
	den.setInternal(mpq_denref(r_value));
	return den;
}
Number Number::complexNumerator() const {
	Number num;
	if(hasImaginaryPart()) num.setInternal(mpq_numref(i_value->internalRational()));
	return num;
}
Number Number::complexDenominator() const {
	Number den(1, 0);
	if(hasImaginaryPart()) den.setInternal(mpq_denref(i_value->internalRational()));
	return den;
}

void Number::operator = (const Number &o) {set(o);}
void Number::operator = (long int i) {set(i, 1);}
void Number::operator -- (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_sub_ui(fl_value, fl_value, 1, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_sub_ui(fu_value, fu_value, 1, MPFR_RNDU);
			mpfr_sub_ui(fl_value, fl_value, 1, MPFR_RNDD);
		}
	}
}
void Number::operator ++ (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_add_ui(fl_value, fl_value, 1, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_add_ui(fu_value, fu_value, 1, MPFR_RNDU);
			mpfr_add_ui(fl_value, fl_value, 1, MPFR_RNDD);
		}
	}
}
Number Number::operator - () const {Number o(*this); o.negate(); return o;}
Number Number::operator * (const Number &o) const {Number o2(*this); o2.multiply(o); return o2;}
Number Number::operator / (const Number &o) const {Number o2(*this); o2.divide(o); return o2;}
Number Number::operator + (const Number &o) const {Number o2(*this); o2.add(o); return o2;}
Number Number::operator - (const Number &o) const {Number o2(*this); o2.subtract(o); return o2;}
Number Number::operator ^ (const Number &o) const {Number o2(*this); o2.raise(o); return o2;}
Number Number::operator * (long int i) const {Number o2(*this); o2.multiply(i); return o2;}
Number Number::operator / (long int i) const {Number o2(*this); o2.divide(i); return o2;}
Number Number::operator + (long int i) const {Number o2(*this); o2.add(i); return o2;}
Number Number::operator - (long int i) const {Number o2(*this); o2.subtract(i); return o2;}
Number Number::operator ^ (long int i) const {Number o2(*this); o2.raise(i); return o2;}
Number Number::operator && (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_AND); return o2;}
Number Number::operator || (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_OR); return o2;}
Number Number::operator ! () const {Number o(*this); o.setLogicalNot(); return o;}
		
void Number::operator *= (const Number &o) {multiply(o);}
void Number::operator /= (const Number &o) {divide(o);}
void Number::operator += (const Number &o) {add(o);}
void Number::operator -= (const Number &o) {subtract(o);}
void Number::operator ^= (const Number &o) {raise(o);}
void Number::operator *= (long int i) {multiply(i);}
void Number::operator /= (long int i) {divide(i);}
void Number::operator += (long int i) {add(i);}
void Number::operator -= (long int i) {subtract(i);}
void Number::operator ^= (long int i) {raise(i);}
	
bool Number::operator == (const Number &o) const {return equals(o);}
bool Number::operator != (const Number &o) const {return !equals(o);}
bool Number::operator < (const Number &o) const {return isLessThan(o);}
bool Number::operator <= (const Number &o) const {return isLessThanOrEqualTo(o);}
bool Number::operator > (const Number &o) const {return isGreaterThan(o);}
bool Number::operator >= (const Number &o) const {return isGreaterThanOrEqualTo(o);}
bool Number::operator == (long int i) const {return equals(i);}
bool Number::operator != (long int i) const {return !equals(i);}
bool Number::operator < (long int i) const {return isLessThan(i);}
bool Number::operator <= (long int i) const {return isLessThanOrEqualTo(i);}
bool Number::operator > (long int i) const {return isGreaterThan(i);}
bool Number::operator >= (long int i) const {return isGreaterThanOrEqualTo(i);}

bool Number::bitAnd(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_and(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitOr(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_ior(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitXor(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	mpz_xor(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitNot() {
	if(!isInteger()) return false;
	mpz_com(mpq_numref(r_value), mpq_numref(r_value));
	return true;
}
bool Number::bitCmp(unsigned int bits) {
	if(!isInteger()) return false;
	if(isNegative()) {
		return negate() && subtract(1);
	}
	for(unsigned int i = 0; i < bits; i++) {
		mpz_combit(mpq_numref(r_value), i);
	}
	return true;
}
bool Number::bitEqv(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	bitXor(o);
	bitNot();
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftLeft(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftRight(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	mpz_fdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shift(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	bool overflow = false;
	long int y = o.lintValue(&overflow);
	if(overflow) return false;
	if(y < 0) mpz_fdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) -y);
	else mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::hasRealPart() const {
	if(isInfinite(true)) return true;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_sgn(r_value) != 0;
	return !mpfr_zero_p(fu_value) || !mpfr_zero_p(fl_value);
}
bool Number::hasImaginaryPart() const {
	return i_value && !i_value->isZero();
}
bool Number::testErrors(int error_level) const {
	if(mpfr_underflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point underflow"), NULL); return true;}
	if(mpfr_overflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point overflow"), NULL); return true;}
	if(mpfr_divby0_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point division by zero exception"), NULL); return true;}
	if(mpfr_nanflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point not a number exception"), NULL); return true;}
	if(mpfr_erangeflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point range exception"), NULL); return true;}
	return false;
}
bool testComplexZero(const Number *this_nr, const Number *i_nr) {
	if(!i_nr) return false;
	if(!this_nr->isInfinite(true) && !i_nr->isInfinite(true) && !i_nr->isZero() && !this_nr->isZero()) {
		if(i_nr->isFloatingPoint() && (!i_nr->isInterval() || !i_nr->isNonZero())) {
			mpfr_t thisf, testf;
			mpfr_inits2(BIT_PRECISION - 10, thisf, testf, NULL);
			bool b = true, b2 = false;
			if(!this_nr->isInterval() || (!mpfr_zero_p(this_nr->internalLowerFloat()) && !mpfr_inf_p(this_nr->internalLowerFloat()))) {
				b2 = true;
				if(this_nr->isFloatingPoint()) {
					mpfr_set(thisf, this_nr->internalLowerFloat(), MPFR_RNDN);
				} else {
					mpfr_set_q(thisf, this_nr->internalRational(), MPFR_RNDN);
				}
				mpfr_add(testf, thisf, i_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, i_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			if(b && this_nr->isInterval() && !mpfr_zero_p(this_nr->internalUpperFloat()) && !mpfr_inf_p(this_nr->internalUpperFloat())) {
				b2 = true;
				mpfr_set(thisf, this_nr->internalUpperFloat(), MPFR_RNDN);
				mpfr_add(testf, thisf, i_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, i_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			mpfr_clears(thisf, testf, NULL);
			if(b && b2) {
				return true;
			}
		}
	}
	return false;
}

bool testComplex(Number *this_nr, Number *i_nr) {
	if(!i_nr) return false;
	if(!this_nr->isInfinite(true) && !i_nr->isInfinite(true) && !i_nr->isZero() && !this_nr->isZero()) {
		if(i_nr->isFloatingPoint() && (!i_nr->isInterval() || !i_nr->isNonZero())) {
			mpfr_t thisf, testf;
			mpfr_inits2(BIT_PRECISION - 10, thisf, testf, NULL);
			bool b = true, b2 = false;
			if(!this_nr->isInterval() || (!mpfr_zero_p(this_nr->internalLowerFloat()) && !mpfr_inf_p(this_nr->internalLowerFloat()))) {
				b2 = true;
				if(this_nr->isFloatingPoint()) {
					mpfr_set(thisf, this_nr->internalLowerFloat(), MPFR_RNDN);
				} else {
					mpfr_set_q(thisf, this_nr->internalRational(), MPFR_RNDN);
				}
				mpfr_add(testf, thisf, i_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, i_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			if(b && this_nr->isInterval() && !mpfr_zero_p(this_nr->internalUpperFloat()) && !mpfr_inf_p(this_nr->internalUpperFloat())) {
				b2 = true;
				mpfr_set(thisf, this_nr->internalUpperFloat(), MPFR_RNDN);
				mpfr_add(testf, thisf, i_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, i_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			mpfr_clears(thisf, testf, NULL);
			if(b && b2) {
				i_nr->clear(true);
				return true;
			}
		}
		if(this_nr->isFloatingPoint() && (!this_nr->isInterval() || !this_nr->realPartIsNonZero())) {
			mpfr_t thisf, testf;
			mpfr_inits2(BIT_PRECISION - 10, thisf, testf, NULL);
			bool b = true, b2 = false;
			if(!this_nr->isInterval() || (!mpfr_zero_p(i_nr->internalLowerFloat()) && !mpfr_inf_p(i_nr->internalLowerFloat()))) {
				b2 = true;
				if(i_nr->isFloatingPoint()) {
					mpfr_set(thisf, i_nr->internalLowerFloat(), MPFR_RNDN);
				} else {
					mpfr_set_q(thisf, i_nr->internalRational(), MPFR_RNDN);
				}
				mpfr_add(testf, thisf, this_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, this_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			if(b && i_nr->isInterval() && !mpfr_zero_p(i_nr->internalUpperFloat()) && !mpfr_inf_p(i_nr->internalUpperFloat())) {
				b2 = true;
				mpfr_set(thisf, i_nr->internalUpperFloat(), MPFR_RNDN);
				mpfr_add(testf, thisf, this_nr->internalLowerFloat(), MPFR_RNDN);
				b = mpfr_equal_p(thisf, testf);
				if(b) {
					mpfr_add(testf, thisf, this_nr->internalUpperFloat(), MPFR_RNDN);
					b = mpfr_equal_p(thisf, testf);
				}
			}
			mpfr_clears(thisf, testf, NULL);
			if(b && b2) {
				this_nr->clearReal();
				return true;
			}
		}
	}
	return false;
}
bool Number::testFloatResult(bool allow_infinite_result, int error_level, bool test_integer) {
	if(mpfr_underflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point underflow"), NULL); return false;}
	if(mpfr_overflow_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point overflow"), NULL); return false;}
	if(mpfr_divby0_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point division by zero exception"), NULL); return false;}
	if(mpfr_erangeflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point range exception"), NULL); return false;}
	if(mpfr_nan_p(fu_value) || mpfr_nan_p(fl_value)) return false;
	if(mpfr_nanflag_p()) {if(error_level) CALCULATOR->error(error_level > 1, _("Floating point not a number exception"), NULL); return false;}
	if(mpfr_inexflag_p()) {
		b_approx = true;
		if(!CREATE_INTERVAL && !isInterval() && (i_precision < 0 || i_precision > FROM_BIT_PRECISION(BIT_PRECISION))) i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	}
	mpfr_clear_flags();
	if(mpfr_inf_p(fl_value) && mpfr_inf_p(fu_value) && mpfr_sgn(fl_value) == mpfr_sgn(fu_value)) {
		if(!allow_infinite_result) return false;
		int sign = mpfr_sgn(fl_value);
		if(sign >= 0) n_type = NUMBER_TYPE_PLUS_INFINITY;
		else if(sign < 0) n_type = NUMBER_TYPE_MINUS_INFINITY;
		mpfr_clears(fl_value, fu_value, NULL);
	} else if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
		if(!allow_infinite_result) return false;
	} else if(mpfr_cmp(fl_value, fu_value) > 0) {
		cout << "fl_value > fu_value: " << endl;
		PRINT_MPFR(fl_value, 10);
		PRINT_MPFR(fu_value, 10);
		mpfr_swap(fl_value, fu_value);
	}
	if(test_integer) testInteger();
	if(!b_imag) testComplex(this, i_value);
	return true;
}
void Number::testInteger() {
	if(isFloatingPoint()) {
		if(mpfr_equal_p(fu_value, fl_value)) {
			if(mpfr_integer_p(fl_value) && mpfr_integer_p(fu_value)) {
				mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDN);
				mpfr_clears(fl_value, fu_value, NULL);
				n_type = NUMBER_TYPE_RATIONAL;
			}
		} else if(mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value)) {
			mpfr_clears(fl_value, fu_value, NULL);
			n_type = NUMBER_TYPE_RATIONAL;
		}
	}
	if(i_value) i_value->testInteger();
}
void Number::setPrecisionAndApproximateFrom(const Number &o) {
	if(o.precision() >= 0 && (i_precision < 0 || o.precision() < i_precision)) i_precision = o.precision();
	if(o.isApproximate()) b_approx = true;
}

bool Number::isComplex() const {
	return i_value && i_value->isNonZero();
}
Number Number::integer() const {
	if(isInteger()) return *this;
	Number nr(*this);
	nr.round();
	return nr;
}
bool Number::isInteger(IntegerType integer_type) const {
	if(n_type != NUMBER_TYPE_RATIONAL || hasImaginaryPart()) return false;
	if(mpz_cmp_ui(mpq_denref(r_value), 1) != 0) return false;
	switch(integer_type) {
		case INTEGER_TYPE_NONE: {return true;}
		case INTEGER_TYPE_SIZE: {}
		case INTEGER_TYPE_UINT: {return mpz_fits_uint_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_SINT: {return mpz_fits_sint_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_ULONG: {return mpz_fits_ulong_p(mpq_numref(r_value)) != 0;}
		case INTEGER_TYPE_SLONG: {return mpz_fits_slong_p(mpq_numref(r_value)) != 0;}
	}
	return true;
}
bool Number::isRational() const {
	return n_type == NUMBER_TYPE_RATIONAL && (!i_value || i_value->isZero());
}
bool Number::realPartIsRational() const {
	return n_type == NUMBER_TYPE_RATIONAL;
}
bool Number::isReal() const {
	return !includesInfinity() && !hasImaginaryPart();
}
bool Number::isFraction() const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		return mpz_cmpabs(mpq_denref(r_value), mpq_numref(r_value)) > 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		bool frac_u = mpfr_cmp_ui(fu_value, 1) < 0 && mpfr_cmp_si(fu_value, -1) > 0;
		bool frac_l = mpfr_cmp_ui(fl_value, 1) < 0 && mpfr_cmp_si(fl_value, -1) > 0;
		return frac_u && frac_l;
	}
	return false;
}
bool Number::isZero() const {
	if(i_value && !i_value->isZero()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isNonZero() const {
	if(i_value && i_value->isNonZero()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) return !mpfr_zero_p(fu_value) && mpfr_sgn(fu_value) == mpfr_sgn(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) != 0;
	return true;
}
bool Number::isOne() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_ui(fu_value, 1) == 0 && mpfr_cmp_ui(fl_value, 1) == 0;}
	return mpz_cmp(mpq_denref(r_value), mpq_numref(r_value)) == 0;
}
bool Number::isTwo() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_ui(fu_value, 2) == 0 && mpfr_cmp_ui(fl_value, 2) == 0;}
	return mpq_cmp_si(r_value, 2, 1) == 0;
}
bool Number::isI() const {
	if(!i_value || !i_value->isOne()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isMinusOne() const {
	if(!isReal()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_cmp_si(fu_value, -1) == 0 && mpfr_cmp_si(fl_value, -1) == 0;}
	return mpq_cmp_si(r_value, -1, 1) == 0;
}
bool Number::isMinusI() const {
	if(!i_value || !i_value->isMinusOne()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_zero_p(fu_value) && mpfr_zero_p(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isNegative() const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fu_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::isNonNegative() const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fl_value) >= 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) >= 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isPositive() const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fl_value) > 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isNonPositive() const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fu_value) <= 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) <= 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNegative() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fu_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsPositive() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(fl_value) > 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNonNegative() const {
	if(n_type == NUMBER_TYPE_FLOAT) {return mpfr_sgn(fl_value) >= 0;}
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) >= 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNonZero() const {
	if(n_type == NUMBER_TYPE_FLOAT) return !mpfr_zero_p(fu_value) && mpfr_sgn(fu_value) == mpfr_sgn(fl_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) != 0;
	return true;
}
bool Number::imaginaryPartIsNegative() const {
	return i_value && i_value->isNegative();
}
bool Number::imaginaryPartIsPositive() const {
	return i_value && i_value->isPositive();
}
bool Number::imaginaryPartIsNonNegative() const {
	return i_value && i_value->isNonNegative();
}
bool Number::imaginaryPartIsNonPositive() const {
	return i_value && i_value->isNonPositive();
}
bool Number::imaginaryPartIsNonZero() const {
	return i_value && i_value->isNonZero();
}
bool Number::hasNegativeSign() const {
	if(hasRealPart()) return realPartIsNegative();
	return imaginaryPartIsNegative();
}
bool Number::hasPositiveSign() const {
	if(hasRealPart()) return realPartIsPositive();
	return imaginaryPartIsPositive();
}
bool Number::equalsZero() const {
	return isZero();
}
bool Number::equals(const Number &o, bool allow_interval, bool allow_infinite) const {
	if(!allow_infinite && (includesInfinity() || o.includesInfinity())) return false;
	if(o.hasImaginaryPart()) {
		if(!i_value || !i_value->equals(*o.internalImaginary(), allow_interval, allow_infinite)) return false;
	} else if(hasImaginaryPart()) {
		return false;
	}
	if(allow_infinite) {
		if(o.isPlusInfinity()) return isPlusInfinity();
		if(o.isMinusInfinity()) return isMinusInfinity();
	}
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalLowerFloat(), r_value) == 0 && mpfr_cmp_q(o.internalUpperFloat(), r_value) == 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return (allow_interval || mpfr_equal_p(fu_value, fl_value)) && mpfr_equal_p(fl_value, o.internalLowerFloat()) && mpfr_equal_p(fu_value, o.internalUpperFloat());
		else return mpfr_cmp_q(fu_value, o.internalRational()) == 0 && mpfr_cmp_q(fl_value, o.internalRational()) == 0;
	}
	return mpq_cmp(r_value, o.internalRational()) == 0;
}
bool Number::equals(long int i) const {
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_cmp_si(fl_value, i) == 0 && mpfr_cmp_si(fu_value, i) == 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpq_cmp_si(r_value, i, 1) == 0;
	return false;
}
int Number::equalsApproximately(const Number &o, int prec) const {
	if(includesInfinity() || o.includesInfinity()) return false;
	if(o.isInterval() && !isInterval()) return o.equalsApproximately(*this, prec);
	if(equals(o)) return true;
	int b = 1;
	if(o.hasImaginaryPart()) {
		if(i_value) {
			b = i_value->equalsApproximately(*o.internalImaginary(), prec);
			if(b == 0) return b;
		} else {
			b = o.internalImaginary()->equalsApproximately(nr_zero, prec);
			if(b == 0) return b;
		}
	} else if(hasImaginaryPart()) {
		b = i_value->equalsApproximately(nr_zero, prec);
		if(b == 0) return b;
	}
	bool prec_choosen = prec >= 0;
	if(prec == EQUALS_PRECISION_LOWEST) {
		prec = PRECISION;
		if(i_precision >= 0 && i_precision < prec) prec = i_precision;
		if(o.precision() >= 0 && o.precision() < prec) prec = o.precision();
	} else if(prec == EQUALS_PRECISION_HIGHEST) {
		prec = i_precision;
		if(o.precision() >= 0 && o.precision() > prec) prec = o.precision();
		if(prec < 0) prec = PRECISION;
	} else if(prec == EQUALS_PRECISION_DEFAULT) {
		prec = PRECISION;
	}
	if(prec_choosen || isApproximate() || o.isApproximate()) {
		int b2 = 0;
		if(isInterval()) {
			if(o.isInterval()) {
				mpfr_t test1, test2, test3, test4;
				mpfr_inits2(::ceil(prec * 3.3219281), test1, test2, test3, test4, NULL);
				if(!isNonZero() || !o.isNonZero()) {
					mpfr_add_ui(test1, fl_value, 1, MPFR_RNDN);
					mpfr_add_ui(test2, fu_value, 1, MPFR_RNDN);
					mpfr_add_ui(test3, o.internalLowerFloat(), 1, MPFR_RNDN);
					mpfr_add_ui(test4, o.internalUpperFloat(), 1, MPFR_RNDN);
				} else {
					mpfr_set(test1, fl_value, MPFR_RNDN);
					mpfr_set(test2, fu_value, MPFR_RNDN);
					mpfr_set(test3, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(test4, o.internalUpperFloat(), MPFR_RNDN);
				}
				if(mpfr_equal_p(test1, test2) && mpfr_equal_p(test2, test3) && mpfr_equal_p(test3, test4)) b2 = 1;
				else if(mpfr_cmp(test1, test4) > 0 || mpfr_cmp(test2, test3) < 0) b2 = 0;
				else b2 = -1;
				mpfr_clears(test1, test2, test3, test4, NULL);
			} else if(!isNonZero() && o.isZero()) {
				mpfr_t test1, test2;
				mpfr_inits2(::ceil(prec * 3.3219281), test1, test2, NULL);
				mpfr_add_ui(test1, fl_value, 1, MPFR_RNDN);
				mpfr_add_ui(test2, fu_value, 1, MPFR_RNDN);
				if(mpfr_equal_p(test1, test2)) b2 = true;
				mpfr_clears(test1, test2, NULL);
			} else {
				mpfr_t test1, test2, test3;
				mpfr_inits2(::ceil(prec * 3.3219281), test1, test2, test3, NULL);
				mpfr_set(test1, fl_value, MPFR_RNDN);
				mpfr_set(test2, fu_value, MPFR_RNDN);
				if(o.isFloatingPoint()) {
					mpfr_set(test3, o.internalLowerFloat(), MPFR_RNDN);
				} else {
					mpfr_set_q(test3, o.internalRational(), MPFR_RNDN);
				}
				if(mpfr_equal_p(test1, test2) && mpfr_equal_p(test2, test3)) b2 = 1;
				else if(mpfr_cmp(test3, test1) < 0 || mpfr_cmp(test3, test2) > 0) b2 = 0;
				else b2 = -1;
				mpfr_clears(test1, test2, test3, NULL);
			}
		} else {
			mpfr_t test1, test2;
			mpfr_inits2(::ceil(prec * 3.3219281), test1, test2, NULL);
			if(n_type == NUMBER_TYPE_FLOAT) {
				mpfr_set(test1, fl_value, MPFR_RNDN);
			} else {
				mpfr_set_q(test1, r_value, MPFR_RNDN);
			}
			if(o.isFloatingPoint()) {
				mpfr_set(test2, o.internalLowerFloat(), MPFR_RNDN);
			} else {
				mpfr_set_q(test2, o.internalRational(), MPFR_RNDN);
			}
			if(mpfr_equal_p(test1, test2)) {
				b2 = 1;
			} else {
				mpfr_add_ui(test1, test1, 1, MPFR_RNDN);
				mpfr_add_ui(test2, test2, 1, MPFR_RNDN);
				if(mpfr_equal_p(test1, test2)) b2 = -1;
			}
			mpfr_clears(test1, test2, NULL);
		}
		if(b2 < 0) b = -1;
		if(b2 == 0) b = 0;
		return b;
	}
	if(b == 1) b = 0;
	return b;
}
ComparisonResult Number::compare(long int i) const {return compare(Number(i, 1));}
ComparisonResult Number::compare(const Number &o, bool ignore_imag) const {
	if(isPlusInfinity()) {
		if((!ignore_imag && o.hasImaginaryPart()) || o.includesPlusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		else return COMPARISON_RESULT_LESS;
	}
	if(isMinusInfinity()) {
		if((!ignore_imag && o.hasImaginaryPart()) || o.includesMinusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) {
		if((!ignore_imag && hasImaginaryPart()) || includesPlusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		return COMPARISON_RESULT_GREATER;
	}
	if(o.isMinusInfinity()) {
		if((!ignore_imag && hasImaginaryPart()) || includesMinusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		return COMPARISON_RESULT_LESS;
	}
	if(!ignore_imag && equals(o)) return COMPARISON_RESULT_EQUAL;
	if(ignore_imag || (!hasImaginaryPart() && !o.hasImaginaryPart())) {
		int i = 0, i2 = 0;
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			i = mpfr_cmp_q(o.internalLowerFloat(), r_value);
			i2 = mpfr_cmp_q(o.internalUpperFloat(), r_value);
			if(i != i2) return COMPARISON_RESULT_CONTAINS;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				i = mpfr_cmp(o.internalUpperFloat(), fl_value);
				i2 = mpfr_cmp(o.internalLowerFloat(), fu_value);
				if(i != i2 && i2 <= 0 && i >= 0) {
					i = mpfr_cmp(o.internalLowerFloat(), fl_value);
					i2 = mpfr_cmp(o.internalUpperFloat(), fu_value);
					if(i > 0) {
						if(i2 <= 0) return COMPARISON_RESULT_CONTAINED;
						else return COMPARISON_RESULT_OVERLAPPING_GREATER;
					} else if(i < 0) {
						if(i2 >= 0) return COMPARISON_RESULT_CONTAINS;
						else return COMPARISON_RESULT_OVERLAPPING_LESS;
					} else {
						if(i2 == 0) return COMPARISON_RESULT_EQUAL_LIMITS;
						else if(i2 > 0) return COMPARISON_RESULT_CONTAINS;
						else return COMPARISON_RESULT_CONTAINED;
					}
				}
			} else {
				i = -mpfr_cmp_q(fl_value, o.internalRational());
				i2 = -mpfr_cmp_q(fu_value, o.internalRational());
				if(i != i2) return COMPARISON_RESULT_CONTAINED;
			}
		} else {
			i = mpq_cmp(o.internalRational(), r_value);
			i2 = i;
		}
		if(i2 == 0 || i == 0) {
			if(i == 0) i = i2;
			if(i > 0) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			else if(i < 0) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i2 != i) {
			return COMPARISON_RESULT_UNKNOWN;
		}
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i > 0) return COMPARISON_RESULT_GREATER;
		else return COMPARISON_RESULT_LESS;
	} else {
		if(hasImaginaryPart()) {
			if(o.hasImaginaryPart()) {
				ComparisonResult cr = realPart().compare(o.realPart());
				if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
				if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {
					cr = imaginaryPart().compare(o.imaginaryPart());
					if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
					return COMPARISON_RESULT_UNKNOWN;
				}
			} else if(!i_value->isNonZero()) {
				ComparisonResult cr = realPart().compare(o.realPart());
				if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
				return COMPARISON_RESULT_UNKNOWN;
			}
		} else if(!o.imaginaryPartIsNonZero()) {
			ComparisonResult cr = realPart().compare(o.realPart());
			if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
			return COMPARISON_RESULT_UNKNOWN;
		}
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareAbsolute(const Number &o, bool ignore_imag) const {
	if(isPositive()) {
		if(o.isPositive()) return compare(o, ignore_imag);
		Number nr(o);
		nr.negate();
		return compare(nr, ignore_imag);
	} else if(o.isPositive()) {
		Number nr(*this);
		nr.negate();
		return nr.compare(o, ignore_imag);
	}
	if(!ignore_imag && (hasImaginaryPart() || o.hasImaginaryPart())) {
		Number nr1(*this);
		nr1.negate();
		Number nr2(o);
		nr2.negate();
		return nr1.compare(nr2, ignore_imag);
	}
	return o.compare(*this, ignore_imag);
}
ComparisonResult Number::compareApproximately(const Number &o, int prec) const {
	if(isPlusInfinity()) {
		if(o.hasImaginaryPart() || o.includesPlusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		else return COMPARISON_RESULT_LESS;
	}
	if(isMinusInfinity()) {
		if(o.hasImaginaryPart() || o.includesMinusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) {
		if(hasImaginaryPart() || includesPlusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		return COMPARISON_RESULT_GREATER;
	}
	if(o.isMinusInfinity()) {
		if(hasImaginaryPart() || includesMinusInfinity()) return COMPARISON_RESULT_UNKNOWN;
		return COMPARISON_RESULT_LESS;
	}
	int b = equalsApproximately(o, prec);
	if(b > 0) return COMPARISON_RESULT_EQUAL;
	else if(b < 0) return COMPARISON_RESULT_UNKNOWN;
	if(!hasImaginaryPart() && !o.hasImaginaryPart()) {
		int i = 0, i2 = 0;
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			i = mpfr_cmp_q(o.internalLowerFloat(), r_value);
			i2 = mpfr_cmp_q(o.internalUpperFloat(), r_value);
			if(i != i2) return COMPARISON_RESULT_CONTAINS;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				i = mpfr_cmp(o.internalUpperFloat(), fl_value);
				i2 = mpfr_cmp(o.internalLowerFloat(), fu_value);
				if(i != i2 && i2 <= 0 && i >= 0) {
					i = mpfr_cmp(o.internalLowerFloat(), fl_value);
					i2 = mpfr_cmp(o.internalUpperFloat(), fu_value);
					if(i > 0) {
						if(i2 <= 0) return COMPARISON_RESULT_CONTAINED;
						else return COMPARISON_RESULT_OVERLAPPING_GREATER;
					} else if(i < 0) {
						if(i2 >= 0) return COMPARISON_RESULT_CONTAINS;
						else return COMPARISON_RESULT_OVERLAPPING_LESS;
					} else {
						if(i2 == 0) return COMPARISON_RESULT_EQUAL_LIMITS;
						else if(i2 > 0) return COMPARISON_RESULT_CONTAINS;
						else return COMPARISON_RESULT_CONTAINED;
					}
				}
			} else {
				i = -mpfr_cmp_q(fl_value, o.internalRational());
				i2 = -mpfr_cmp_q(fu_value, o.internalRational());
				if(i != i2) return COMPARISON_RESULT_CONTAINED;
			}
		} else {
			i = mpq_cmp(o.internalRational(), r_value);
			i2 = i;
		}
		if(i2 == 0 || i == 0) {
			if(i == 0) i = i2;
			if(i > 0) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			else if(i < 0) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i2 != i) {
			return COMPARISON_RESULT_UNKNOWN;
		}
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i > 0) return COMPARISON_RESULT_GREATER;
		else return COMPARISON_RESULT_LESS;
	} else {		
		if(hasImaginaryPart()) {
			if(o.hasImaginaryPart()) {
				ComparisonResult cr = realPart().compareApproximately(o.realPart());
				if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
				if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {
					cr = imaginaryPart().compareApproximately(o.imaginaryPart());
					if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
					return COMPARISON_RESULT_UNKNOWN;
				}
			} else if(!i_value->isNonZero()) {
				ComparisonResult cr = realPart().compareApproximately(o.realPart());
				if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
				return COMPARISON_RESULT_UNKNOWN;
			}
		} else if(!o.imaginaryPartIsNonZero()) {
			ComparisonResult cr = realPart().compareApproximately(o.realPart());
			if(COMPARISON_IS_NOT_EQUAL(cr)) return COMPARISON_RESULT_NOT_EQUAL;
			return COMPARISON_RESULT_UNKNOWN;
		}
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareImaginaryParts(const Number &o) const {
	if(o.hasImaginaryPart()) {
		if(!i_value) {
			if(o.imaginaryPartIsNonZero()) return COMPARISON_RESULT_NOT_EQUAL;
			return COMPARISON_RESULT_UNKNOWN;
		}
		return i_value->compareRealParts(*o.internalImaginary());
	} else if(hasImaginaryPart()) {
		if(i_value->isNonZero()) return COMPARISON_RESULT_NOT_EQUAL;
		return COMPARISON_RESULT_NOT_EQUAL;
	}
	return COMPARISON_RESULT_EQUAL;
}
ComparisonResult Number::compareRealParts(const Number &o) const {
	return compare(o, true);
}
bool Number::isGreaterThan(const Number &o) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return false;
	if(o.isMinusInfinity()) return true;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(hasImaginaryPart() || o.hasImaginaryPart()) return false;
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalUpperFloat(), r_value) < 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return mpfr_greater_p(fl_value, o.internalUpperFloat());
		else return mpfr_cmp_q(fl_value, o.internalRational()) > 0;
	}
	return mpq_cmp(r_value, o.internalRational()) > 0;
}
bool Number::isLessThan(const Number &o) const {
	if(o.isMinusInfinity() || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return true;
	if(hasImaginaryPart() || o.hasImaginaryPart()) return false;
	if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalLowerFloat(), r_value) > 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isFloatingPoint()) return mpfr_less_p(fu_value, o.internalLowerFloat());
		else return mpfr_cmp_q(fu_value, o.internalRational()) < 0;
	}
	return mpq_cmp(r_value, o.internalRational()) < 0;
}
bool Number::isGreaterThanOrEqualTo(const Number &o) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return false;
	if(o.isMinusInfinity()) return true;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(!hasImaginaryPart() && !o.hasImaginaryPart()) {
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalUpperFloat(), r_value) <= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) return mpfr_greaterequal_p(fl_value, o.internalUpperFloat());
			else return mpfr_cmp_q(fl_value, o.internalRational()) >= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) >= 0;
	}
	return false;
}
bool Number::isLessThanOrEqualTo(const Number &o) const {
	if(o.isMinusInfinity() || n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || o.isPlusInfinity()) return true;
	if(!hasImaginaryPart() && !o.hasImaginaryPart()) {
		if(o.isFloatingPoint() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalLowerFloat(), r_value) >= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) return mpfr_lessequal_p(fu_value, o.internalLowerFloat());
			else return mpfr_cmp_q(fu_value, o.internalRational()) <= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) <= 0;
	}
	return false;
}
bool Number::isGreaterThan(long int i) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fl_value, i) > 0;
	}
	return mpq_cmp_si(r_value, i, 1) > 0;
}
bool Number::isLessThan(long int i) const {
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fu_value, i) < 0;
	}
	return mpq_cmp_si(r_value, i, 1) < 0;
}
bool Number::isGreaterThanOrEqualTo(long int i) const {
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fl_value, i) >= 0;
	}
	return mpq_cmp_si(r_value, i, 1) >= 0;
}
bool Number::isLessThanOrEqualTo(long int i) const {
	if(n_type == NUMBER_TYPE_PLUS_INFINITY) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	if(hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_si(fu_value, i) <= 0;
	}
	return mpq_cmp_si(r_value, i, 1) <= 0;
}
bool Number::numeratorIsGreaterThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) > 0;
}
bool Number::numeratorIsLessThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) < 0;
}
bool Number::numeratorEquals(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_numref(r_value), i) == 0;
}
bool Number::denominatorIsGreaterThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) > 0;
}
bool Number::denominatorIsLessThan(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) < 0;
}
bool Number::denominatorEquals(long int i) const {
	if(!isRational()) return false;
	return mpz_cmp_si(mpq_denref(r_value), i) == 0;
}
bool Number::denominatorIsGreater(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) > 0;
}
bool Number::denominatorIsLess(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) < 0;
}
bool Number::denominatorIsEqual(const Number &o) const {
	if(!isRational() || !o.isRational()) return false;
	return mpz_cmp(mpq_denref(r_value), mpq_denref(o.internalRational())) == 0;
}
bool Number::isEven() const {
	return isInteger() && mpz_even_p(mpq_numref(r_value));
}
bool Number::denominatorIsEven() const {
	return !hasImaginaryPart() && n_type == NUMBER_TYPE_RATIONAL && mpz_even_p(mpq_denref(r_value));
}
bool Number::denominatorIsTwo() const {
	return !hasImaginaryPart() && n_type == NUMBER_TYPE_RATIONAL && mpz_cmp_si(mpq_denref(r_value), 2) == 0;
}
bool Number::numeratorIsEven() const {
	return !hasImaginaryPart() && n_type == NUMBER_TYPE_RATIONAL && mpz_even_p(mpq_numref(r_value));
}
bool Number::numeratorIsOne() const {
	return !hasImaginaryPart() && n_type == NUMBER_TYPE_RATIONAL && mpz_cmp_si(mpq_numref(r_value), 1) == 0;
}
bool Number::numeratorIsMinusOne() const {
	return !hasImaginaryPart() && n_type == NUMBER_TYPE_RATIONAL && mpz_cmp_si(mpq_numref(r_value), -1) == 0;
}
bool Number::isOdd() const {
	return isInteger() && mpz_odd_p(mpq_numref(r_value));
}

int Number::integerLength() const {
	if(isInteger()) return mpz_sizeinbase(mpq_numref(r_value), 2);
	return 0;
}


bool Number::add(const Number &o) {
	if(n_type == NUMBER_TYPE_RATIONAL && o.realPartIsRational()) {
		if(o.hasImaginaryPart()) {
			if(!i_value) {i_value = new Number(*o.internalImaginary()); i_value->markAsImaginaryPart();}
			else if(!i_value->add(*o.internalImaginary())) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_add(r_value, r_value, o.internalRational());
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number nr_bak(*this);
	if(o.hasImaginaryPart()) {
		if(!i_value) {i_value = new Number(*o.internalImaginary()); i_value->markAsImaginaryPart();}
		else if(!i_value->add(*o.internalImaginary())) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	if(includesMinusInfinity() && o.includesPlusInfinity()) return false;
	if(includesPlusInfinity() && o.includesMinusInfinity()) return false;
	if(isInfinite(true)) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isPlusInfinity(true)) {
		setPlusInfinity(true, true);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity(true)) {
		setMinusInfinity(true, true);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear_flags();
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION);
			mpfr_init2(fl_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			if(!CREATE_INTERVAL && !o.isInterval()) {
				mpfr_add_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_add_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
				mpfr_add_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
			}
			mpq_set_ui(r_value, 0, 1);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				if(!CREATE_INTERVAL && !isInterval() && !o.isInterval()) {
					mpfr_add(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_add(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
					mpfr_add(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
				}
			} else {
				if(!CREATE_INTERVAL && !isInterval()) {
					mpfr_add_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_add_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
					mpfr_add_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
				}
			}
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::add(long int i) {
	if(i == 0) return true;
	if(isInfinite(true)) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_add_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_add_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_add_si(fl_value, fl_value, i, MPFR_RNDD);
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(i < 0) mpz_submul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) (-i));
		else mpz_addmul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) i);
	}
	return true;
}

bool Number::subtract(const Number &o) {
	if(n_type == NUMBER_TYPE_RATIONAL && o.realPartIsRational()) {
		if(o.hasImaginaryPart()) {
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			if(!i_value->subtract(*o.internalImaginary())) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_sub(r_value, r_value, o.internalRational());
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number nr_bak(*this);
	if(includesPlusInfinity() && o.includesPlusInfinity()) return false;
	if(includesMinusInfinity() && o.includesMinusInfinity()) return false;
	if(o.hasImaginaryPart()) {
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		if(!i_value->subtract(*o.internalImaginary())) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	if(isInfinite()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isPlusInfinity(true)) {
		setMinusInfinity(true, true);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity(true)) {
		setPlusInfinity(true, true);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		mpfr_clear_flags();
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION);
			mpfr_init2(fl_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			if(!CREATE_INTERVAL && !o.isInterval()) {
				mpfr_sub_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_sub_q(fu_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
				mpfr_neg(fu_value, fu_value, MPFR_RNDU);
				mpfr_sub_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
				mpfr_neg(fl_value, fl_value, MPFR_RNDD);
			}
			mpq_set_ui(r_value, 0, 1);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isFloatingPoint()) {
				if(!CREATE_INTERVAL && !isInterval() && !o.isInterval()) {
					mpfr_sub(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_sub(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
					mpfr_sub(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
				}
			} else {
				if(!CREATE_INTERVAL && !isInterval()) {
					mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_sub_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
					mpfr_sub_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
				}
			}
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::subtract(long int i) {
	if(i == 0) return true;
	if(isInfinite(true)) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		mpfr_clear_flags();
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_sub_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_sub_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_sub_si(fl_value, fl_value, i, MPFR_RNDD);
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(i < 0) mpz_addmul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) (-i));
		else mpz_submul_ui(mpq_numref(r_value), mpq_denref(r_value), (unsigned int) i);
	}
	return true;
}

bool Number::multiply(const Number &o) {
	if(o.hasImaginaryPart()) {
		if(o.hasRealPart()) {
			Number nr_copy;
			if(hasImaginaryPart()) {
				if(hasRealPart()) {
					Number nr_real;
					nr_real.set(*this, false, true);
					nr_copy.set(*i_value);
					if(!nr_real.multiply(o.realPart()) || !nr_copy.multiply(o.imaginaryPart()) || !nr_copy.negate() || !nr_real.add(nr_copy)) return false;
					Number nr_imag(*i_value);
					nr_copy.set(*this, false, true);
					if(!nr_copy.multiply(o.imaginaryPart()) || !nr_imag.multiply(o.realPart()) || !nr_imag.add(nr_copy)) return false;
					set(nr_real, true, true);
					i_value->set(nr_imag, true);
					setPrecisionAndApproximateFrom(*i_value);
					testComplex(this, i_value);
					return true;
				}
				nr_copy.set(*i_value);
				nr_copy.negate();
			} else if(hasRealPart()) {
				nr_copy.setImaginaryPart(*this);
			}
			Number nr_bak(*this);
			if(!nr_copy.multiply(o.imaginaryPart())) return false;
			if(!multiply(o.realPart())) return false;
			if(!add(nr_copy)) {
				set(nr_bak);
				return false;
			}
			return true;
		} else if(hasImaginaryPart()) {
			Number nr_copy(*i_value);
			nr_copy.negate();
			if(hasRealPart()) {
				nr_copy.setImaginaryPart(*this);
			}
			if(!nr_copy.multiply(o.imaginaryPart())) return false;
			set(nr_copy);
			return true;
		}
		if(!multiply(*o.internalImaginary())) return false;
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->set(*this, true, true);
		clearReal();
		return true;
	}
	if(o.includesInfinity() && !isNonZero()) return false;
	if(includesInfinity() && !o.isNonZero()) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(o.isNegative()) {
			if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			} else {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			}
			setPrecisionAndApproximateFrom(o);
		}
		return true;
	}
	if(o.isPlusInfinity()) {
		if(hasRealPart() && !realPartIsNonZero()) return false;
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(hasRealPart()) {
			if(isNegative()) setMinusInfinity(true, true);
			else setPlusInfinity(true, true);
			setPrecisionAndApproximateFrom(o);
		}
		return true;
	}
	if(o.isMinusInfinity()) {
		if(hasRealPart() && !realPartIsNonZero()) return false;
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(hasRealPart()) {
			if(isNegative()) setPlusInfinity(true, true);
			else setMinusInfinity(true, true);
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isZero()) return true;
	if(o.isZero()) {
		clear();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(hasRealPart()) {
			mpfr_clear_flags();
			if(n_type != NUMBER_TYPE_FLOAT) {
				mpfr_init2(fu_value, BIT_PRECISION);
				mpfr_init2(fl_value, BIT_PRECISION);
				n_type = NUMBER_TYPE_FLOAT;
				if(!CREATE_INTERVAL && !o.isInterval()) {
					mpfr_mul_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					if(mpq_sgn(r_value) < 0) {
						mpfr_mul_q(fu_value, o.internalLowerFloat(), r_value, MPFR_RNDU);
						mpfr_mul_q(fl_value, o.internalUpperFloat(), r_value, MPFR_RNDD);
					} else {
						mpfr_mul_q(fu_value, o.internalUpperFloat(), r_value, MPFR_RNDU);
						mpfr_mul_q(fl_value, o.internalLowerFloat(), r_value, MPFR_RNDD);
					}
				}
				mpq_set_ui(r_value, 0, 1);
			} else if(n_type == NUMBER_TYPE_FLOAT) {
				if(o.isFloatingPoint()) {
					if(!CREATE_INTERVAL && !isInterval() && !o.isInterval()) {
						mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
						mpfr_set(fu_value, fl_value, MPFR_RNDN);
					} else {
						int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value), sgn_ol = mpfr_sgn(o.internalLowerFloat()), sgn_ou = mpfr_sgn(o.internalUpperFloat());
						if((sgn_l < 0) != (sgn_u < 0)) {
							if((sgn_ol < 0) != (sgn_ou < 0)) {
								mpfr_t fu_value2, fl_value2;
								mpfr_init2(fu_value2, BIT_PRECISION);
								mpfr_init2(fl_value2, BIT_PRECISION);
								mpfr_mul(fu_value2, fl_value, o.internalLowerFloat(), MPFR_RNDU);
								mpfr_mul(fl_value2, fu_value, o.internalLowerFloat(), MPFR_RNDD);
								mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
								mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
								if(mpfr_cmp(fu_value, fu_value2) < 0) mpfr_set(fu_value, fu_value2, MPFR_RNDU);
								if(mpfr_cmp(fl_value, fl_value2) > 0) mpfr_set(fl_value, fl_value2, MPFR_RNDD);
								mpfr_clears(fu_value2, fl_value2, NULL);
							} else if(sgn_ol < 0) {
								mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
								mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
								mpfr_swap(fu_value, fl_value);
							} else {
								mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
								mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
							}
						} else if((sgn_ol < 0) != (sgn_ou < 0)) {
							if(sgn_l < 0) {
								mpfr_mul(fu_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
								mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
							} else {
								mpfr_mul(fl_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
								mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
							}
						} else if(sgn_l < 0) {
							if(sgn_ol < 0) {
								mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
								mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDD);
								mpfr_swap(fl_value, fu_value);
							} else {
								mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
								mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
							}
						} else if(sgn_ol < 0) {
							mpfr_mul(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDU);
							mpfr_mul(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
							mpfr_swap(fu_value, fl_value);
						} else {
							mpfr_mul(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
							mpfr_mul(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
						}
					}
				} else {
					if(!CREATE_INTERVAL && !isInterval()) {
						mpfr_mul_q(fl_value, fl_value, o.internalRational(), MPFR_RNDN);
						mpfr_set(fu_value, fl_value, MPFR_RNDN);
					} else if(mpq_sgn(o.internalRational()) < 0) {
						mpfr_mul_q(fu_value, fu_value, o.internalRational(), MPFR_RNDD);
						mpfr_mul_q(fl_value, fl_value, o.internalRational(), MPFR_RNDU);
						mpfr_swap(fu_value, fl_value);
					} else {
						mpfr_mul_q(fu_value, fu_value, o.internalRational(), MPFR_RNDU);
						mpfr_mul_q(fl_value, fl_value, o.internalRational(), MPFR_RNDD);
					}
				}
			}
			if(!testFloatResult(true)) {
				set(nr_bak);
				return false;
			}
		}
	} else {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(o)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_mul(r_value, r_value, o.internalRational());
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::multiply(long int i) {
	if(i == 0 && includesInfinity()) return false;
	if(n_type == NUMBER_TYPE_MINUS_INFINITY || n_type == NUMBER_TYPE_PLUS_INFINITY) {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(i < 0) {
			if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			} else {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			}
		}
		return true;
	}
	if(isZero()) return true;
	if(i == 0) {
		clear();
		return true;
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		if(hasImaginaryPart()) {
			if(!i_value->multiply(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpfr_clear_flags();

		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_mul_si(fl_value, fl_value, i, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_mul_si(fu_value, fu_value, i, MPFR_RNDU);
			mpfr_mul_si(fl_value, fl_value, i, MPFR_RNDD);
			if(i < 0) mpfr_swap(fu_value, fl_value);
		}
		
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	} else {
		if(hasImaginaryPart()) {
			if(!i_value->multiply(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		mpq_t r_i;
		mpq_init(r_i);
		mpz_set_si(mpq_numref(r_i), i);
		mpq_mul(r_value, r_value, r_i);
		mpq_clear(r_i);
	}
	return true;
}

bool Number::divide(const Number &o) {
	if(isInfinite() || o.isInfinite() || o.hasImaginaryPart() || o.isFloatingPoint() || n_type == NUMBER_TYPE_FLOAT) {
		Number oinv(o);
		if(!oinv.recip()) return false;
		return multiply(oinv);
	}
	if(!o.isNonZero()) {
		if(isZero()) return false;
		return false;
	}
	if(isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(hasImaginaryPart()) {
		if(!i_value->divide(o)) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	mpq_div(r_value, r_value, o.internalRational());
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::divide(long int i) {
	if(includesInfinity() && i == 0) return false;
	if(isInfinite(true)) {
		if(hasImaginaryPart()) {
			if(!i_value->divide(i)) return false;
			setPrecisionAndApproximateFrom(*i_value);
		}
		if(i < 0) {
			if(n_type == NUMBER_TYPE_PLUS_INFINITY) {
				n_type = NUMBER_TYPE_MINUS_INFINITY;
			} else if(n_type == NUMBER_TYPE_MINUS_INFINITY) {
				n_type = NUMBER_TYPE_PLUS_INFINITY;
			}
		}
		return true;
	}
	if(i == 0) return false;
	if(isZero()) return true;
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number oinv(i < 0 ? -1 : 1, i < 0 ? -i : i, 0);
		return multiply(oinv);
	}
	if(hasImaginaryPart()) {
		if(!i_value->divide(i)) return false;
		setPrecisionAndApproximateFrom(*i_value);
	}
	mpq_t r_i;
	mpq_init(r_i);
	mpz_set_si(mpq_numref(r_i), i);
	mpq_div(r_value, r_value, r_i);
	mpq_clear(r_i);
	return true;
}

bool Number::recip() {
	if(!isNonZero()) {
		return false;
	}
	if(isInfinite(false)) {
		clear();
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			if(isInterval(false)) {
			
				// 1/(x+yi) = (x-yi)/(x^2+y^2); max/min x/(x^2+y^2): x=abs(y), x=-abs(y) 
			
				Number nr_bak(*this);
				if(!setToFloatingPoint()) return false;
				if(!i_value->setToFloatingPoint()) return false;
				mpfr_t abs_il, abs_iu, absm_iu, absm_il, abs_rl, abs_ru, absm_ru, absm_rl, ftmp1, ftmp2, fu_tmp, fl_tmp;
				mpfr_inits2(BIT_PRECISION, abs_il, abs_iu, absm_iu, absm_il, abs_rl, abs_ru, absm_ru, absm_rl, ftmp1, ftmp2, fu_tmp, fl_tmp, NULL);

				if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) mpfr_set_zero(abs_rl, 0);
				else if(mpfr_cmpabs(fu_value, fl_value) < 0) mpfr_abs(abs_rl, fu_value, MPFR_RNDD);
				else mpfr_abs(abs_rl, fl_value, MPFR_RNDD);
				if(mpfr_cmpabs(fl_value, fu_value) > 0) mpfr_abs(abs_ru, fl_value, MPFR_RNDU);
				else mpfr_abs(abs_ru, fu_value, MPFR_RNDU);
				mpfr_neg(absm_ru, abs_ru, MPFR_RNDD);
				mpfr_neg(absm_rl, abs_rl, MPFR_RNDD);
				
				if(mpfr_sgn(i_value->internalLowerFloat()) != mpfr_sgn(i_value->internalUpperFloat())) mpfr_set_zero(abs_il, 0);
				else if(mpfr_cmpabs(i_value->internalUpperFloat(), i_value->internalLowerFloat()) < 0) mpfr_abs(abs_il, i_value->internalUpperFloat(), MPFR_RNDD);
				else mpfr_abs(abs_il, i_value->internalLowerFloat(), MPFR_RNDD);
				if(mpfr_cmpabs(i_value->internalLowerFloat(), i_value->internalUpperFloat()) > 0) mpfr_abs(abs_iu, i_value->internalLowerFloat(), MPFR_RNDU);
				else mpfr_abs(abs_iu, i_value->internalUpperFloat(), MPFR_RNDU);
				mpfr_neg(absm_iu, abs_iu, MPFR_RNDD);
				mpfr_neg(absm_il, abs_il, MPFR_RNDD);
				
				for(size_t i = 0; i < 2; i++) {
					if(i == 0) {
						mpfr_swap(fu_tmp, fu_value);
						mpfr_swap(fl_tmp, fl_value);
					} else {
						mpfr_swap(fu_tmp, i_value->internalUpperFloat());
						mpfr_swap(fl_tmp, i_value->internalLowerFloat());
						mpfr_swap(abs_il, abs_rl);
						mpfr_swap(abs_iu, abs_ru);
						mpfr_swap(absm_iu, absm_ru);
						mpfr_swap(absm_il, absm_rl);
						mpfr_neg(fu_tmp, fu_tmp, MPFR_RNDU);
						mpfr_neg(fl_tmp, fl_tmp, MPFR_RNDD);
						mpfr_swap(fl_tmp, fu_tmp);
					}
					
					bool neg = mpfr_sgn(fu_tmp) < 0;
					if(neg) {
						mpfr_neg(fu_tmp, fu_tmp, MPFR_RNDD);
						mpfr_neg(fl_tmp, fl_tmp, MPFR_RNDU);
						mpfr_swap(fl_tmp, fu_tmp);
					}
					if(mpfr_cmp(fl_tmp, abs_il) <= 0) {
						if(mpfr_cmp(fu_tmp, abs_il) >= 0) {
							mpfr_sqr(ftmp1, abs_il, MPFR_RNDD);
							mpfr_mul_ui(ftmp1, ftmp1, 2, MPFR_RNDD);
							mpfr_div(fu_tmp, abs_il, ftmp1, MPFR_RNDU);
						} else {
							mpfr_sqr(ftmp1, fu_tmp, MPFR_RNDD);
							mpfr_sqr(ftmp2, abs_il, MPFR_RNDD);
							mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDD);
							mpfr_div(fu_tmp, fu_tmp, ftmp1, MPFR_RNDU);
						}
					} else {
						mpfr_sqr(ftmp1, fl_tmp, MPFR_RNDD);
						mpfr_sqr(ftmp2, abs_il, MPFR_RNDD);
						mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDD);
						mpfr_div(fu_tmp, fl_tmp, ftmp1, MPFR_RNDU);
					}
					if(mpfr_sgn(fl_tmp) < 0) {
						if(mpfr_cmp(fl_tmp, absm_il) <= 0) {
							mpfr_sqr(ftmp1, abs_il, MPFR_RNDU);
							mpfr_mul_ui(ftmp1, ftmp1, 2, MPFR_RNDU);
							mpfr_div(fl_tmp, absm_il, ftmp1, MPFR_RNDD);
						} else {
							mpfr_sqr(ftmp1, fl_tmp, MPFR_RNDD);
							mpfr_sqr(ftmp2, abs_il, MPFR_RNDD);
							mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDD);
							mpfr_div(fl_tmp, fl_tmp, ftmp1, MPFR_RNDU);
						}
					} else if(mpfr_cmp(fl_tmp, absm_iu) <= 0) {
						if(mpfr_cmp(abs_ru, absm_iu) >= 0) {
							mpfr_sqr(ftmp1, abs_iu, MPFR_RNDU);
							mpfr_mul_ui(ftmp1, ftmp1, 2, MPFR_RNDU);
							mpfr_div(fl_tmp, absm_iu, ftmp1, MPFR_RNDD);
						} else {
							mpfr_sqr(ftmp1, abs_ru, MPFR_RNDU);
							mpfr_sqr(ftmp2, abs_iu, MPFR_RNDU);
							mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDU);
							mpfr_div(fl_tmp, abs_ru, ftmp1, MPFR_RNDD);
						}
					} else {
						if(mpfr_cmp(fl_tmp, abs_iu) > 0) {
							mpfr_sqr(ftmp1, abs_ru, MPFR_RNDU);
							mpfr_sqr(ftmp2, abs_iu, MPFR_RNDU);
							mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDU);
							mpfr_div(fl_tmp, abs_ru, ftmp1, MPFR_RNDD);
						} else {
							mpfr_sqr(ftmp1, abs_rl, MPFR_RNDU);
							mpfr_sqr(ftmp2, abs_iu, MPFR_RNDU);
							mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDU);
							mpfr_div(fl_tmp, fl_tmp, ftmp1, MPFR_RNDD);
							if(mpfr_cmp(abs_ru, abs_iu) > 0) {
								mpfr_sqr(ftmp1, abs_ru, MPFR_RNDU);
								mpfr_sqr(ftmp2, abs_iu, MPFR_RNDU);
								mpfr_add(ftmp1, ftmp1, ftmp2, MPFR_RNDU);
								mpfr_div(ftmp1, abs_ru, ftmp1, MPFR_RNDD);
								if(mpfr_cmp(ftmp1, fl_tmp) < 0) {
									mpfr_swap(ftmp1, fl_tmp);
								}
							}
						}
					}
					if(neg) {
						mpfr_neg(fu_tmp, fu_tmp, MPFR_RNDD);
						mpfr_neg(fl_tmp, fl_tmp, MPFR_RNDU);
						mpfr_swap(fl_tmp, fu_tmp);
					}
					
					if(i == 0) {
						mpfr_swap(fu_tmp, fu_value);
						mpfr_swap(fl_tmp, fl_value);
					} else {
						mpfr_swap(fu_tmp, i_value->internalUpperFloat());
						mpfr_swap(fl_tmp, i_value->internalLowerFloat());
					}
				}
				mpfr_clears(abs_il, abs_iu, absm_iu, absm_il, abs_rl, abs_ru, absm_ru, absm_rl, ftmp1, ftmp2, fu_tmp, fl_tmp, NULL);
				if(!i_value->testFloatResult(true) || !testFloatResult(true)) {
					set(nr_bak);
					return false;
				}
				return true;
			} else {
				Number den1(*i_value);
				Number den2;
				den2.set(*this, false, true);
				Number num_r(den2), num_i(den1);
				if(!den1.square() || !num_i.negate() || !den2.square() || !den1.add(den2) || !num_r.divide(den1) || !num_i.divide(den1)) return false;
				set(num_r);
				setImaginaryPart(num_i);
			}
			return true;
		}
		if(!i_value->recip() || !i_value->negate()) return false;
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		Number nr_bak(*this);
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_ui_div(fl_value, 1, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_ui_div(fu_value, 1, fu_value, MPFR_RNDD);
			mpfr_ui_div(fl_value, 1, fl_value, MPFR_RNDU);
			mpfr_swap(fu_value, fl_value);
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
	} else {
		mpq_inv(r_value, r_value);
	}
	return true;
}
bool Number::raise(const Number &o, bool try_exact) {
	if(o.isTwo()) return square();
	if(o.isMinusOne()) {
		if(!recip()) return false;
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if((includesInfinity(true) || o.includesInfinity(true)) && !hasImaginaryPart() && !o.hasImaginaryPart()) {
		if(isInfinite(false)) {
			if(o.isNegative()) {
				clear(true);
				return true;
			}
			if(!o.isNonZero() || o.hasImaginaryPart()) {
				return false;
			}
			if(isMinusInfinity()) {
				if(o.isEven()) {
					n_type = NUMBER_TYPE_PLUS_INFINITY;
				} else if(!o.isInteger()) {
					return false;
				}
			}
			setPrecisionAndApproximateFrom(o);
			return true;
		}
		if(includesInfinity()) {
			if(hasImaginaryPart() || !o.isNonZero() || o.hasImaginaryPart()) {
				return false;
			}
		}
		if(o.includesMinusInfinity()) {
			if(o.isFloatingPoint() && o.includesPlusInfinity()) return false;
			if(!isNonZero()) {
				return false;
			} else if(isNegative()) {
				if(!isLessThan(-1)) return false;
				if(!o.isFloatingPoint()) clear(true);
			} else if(hasImaginaryPart()) {
				if(hasRealPart()) return false;
				if(!o.isFloatingPoint() && !i_value->raise(o)) return false;
			} else if(isGreaterThan(1)) {
				if(!o.isFloatingPoint()) clear(true);
			} else if(isPositive() && isLessThan(1)) {
				setPlusInfinity();
			} else {
				return false;
			}
			if(!o.isFloatingPoint()) {
				setPrecisionAndApproximateFrom(o);
				return true;
			}
		} else if(o.includesPlusInfinity()) {
			if(!isNonZero()) {
				return false;
			} else if(isNegative()) {
				if(!isGreaterThan(-1)) return false;
				if(!o.isFloatingPoint()) clear(true);
			} else if(hasImaginaryPart()) {
				if(hasRealPart()) return false;
				if(!o.isFloatingPoint() && !i_value->raise(o)) return false;
			} else if(isGreaterThan(1)) {
				setPlusInfinity();
			} else if(isPositive() && isLessThan(1)) {
				if(!o.isFloatingPoint()) clear(true);
			} else {
				return false;
			}
			if(!o.isFloatingPoint()) {
				setPrecisionAndApproximateFrom(o);
				return true;
			}
		}
	}
	if(isZero() && o.isNegative()) {
		CALCULATOR->error(true, _("Division by zero."), NULL);
		return false;
	}
	if(isZero()) {
		if(o.isZero()) {
			//0^0
			CALCULATOR->error(false, _("0^0 might be considered undefined"), NULL);
			set(1, 1, 0, true);
			setPrecisionAndApproximateFrom(o);
			return true;
		} else if(!o.realPartIsNonNegative()) {
			return false;
		} else if(o.hasImaginaryPart()) {
			CALCULATOR->error(false, _("The result of 0^i is possibly undefined"), NULL);
		}
		return true;
	}
	
	if(o.isZero()) {
		if(hasImaginaryPart() && i_value->includesInfinity()) return false;
		set(1, 1, 0, false);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isOne()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isOne() && !o.includesInfinity()) {
		return true;
	}
	if(o.hasImaginaryPart()) {
		if(b_imag) return false;
		Number nr_a, nr_b, nr_c, nr_d;
		if(hasImaginaryPart()) {
			if(hasRealPart()) nr_a = realPart();
			nr_b = imaginaryPart();
		} else {
			nr_a.set(*this);
		}
		if(o.hasRealPart()) nr_c = o.realPart();
		nr_d = o.imaginaryPart();
		Number a2b2c2(1, 1);
		Number a2b2(nr_a);
		Number b2(nr_b);
		if(!a2b2.square() || !b2.square() || !a2b2.add(b2)) return false;
		if(!nr_c.isZero()) {
			Number chalf(nr_c);
			a2b2c2 = a2b2;
			if(!chalf.multiply(nr_half) || !a2b2c2.raise(chalf)) return false;
		}
		Number nr_arg(nr_b);
		if(!nr_arg.atan2(nr_a, true)) return false;
		Number eraised, nexp(nr_d);
		eraised.e();
		if(!nexp.negate() || !nexp.multiply(nr_arg) || !eraised.raise(nexp, false)) return false;
		
		if(!nr_arg.multiply(nr_c) || !nr_d.multiply(nr_half) || !a2b2.ln() || !nr_d.multiply(a2b2) || !nr_arg.add(nr_d)) return false;
		Number nr_cos(nr_arg);
		Number nr_sin(nr_arg);
		if(!nr_cos.cos() || !nr_sin.sin() || !nr_sin.multiply(nr_one_i) || !nr_cos.add(nr_sin)) return false;
		if(!eraised.multiply(a2b2c2) || !eraised.multiply(nr_cos)) return false;
		if((hasImaginaryPart() || o.hasRealPart()) && eraised.isInterval(false) && eraised.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
		set(eraised);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(hasImaginaryPart()) {
		if(o.isNegative()) {
			if(o.isMinusOne()) return recip();
			Number ninv(*this), opos(o);
			if(!ninv.recip() ||!opos.negate() || !ninv.raise(opos, try_exact)) return false;
			set(ninv);
			return true;
		}// else if(!o.isNonNegative()) return false;
		if(hasRealPart() || !o.isInteger()) {
			if(try_exact && !isFloatingPoint() && !i_value->isFloatingPoint() && o.isInteger() && o.isPositive() && o.isLessThan(100)) {
				int i = o.intValue();
				Number nr_init(*this);
				while(i > 1) {
					if(CALCULATOR->aborted()) return false;
					if(!multiply(nr_init)) {
						set(nr_init);
						return false;
					}
					i--;
				}
				setPrecisionAndApproximateFrom(o);
				return true;
			}
			Number nbase, nexp(*this);
			nbase.e();
			if(!nexp.ln() || !nexp.multiply(o) || !nbase.raise(nexp, false)) return false;
			set(nbase);
			return true;
		}
		if(!i_value->raise(o, try_exact)) return false;
		Number ibneg(o);
		if(!ibneg.iquo(2)) return false;
		if(!ibneg.isEven() && !i_value->negate()) return false;
		if(o.isEven()) {
			set(*i_value, true, true);
			clearImaginary();
		} else {
			setPrecisionAndApproximateFrom(*i_value);
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}

	
	if(isMinusOne() && o.isRational()) {
		if(o.isInteger()) {
			if(o.isEven()) set(1, 1, 0, true);
			setPrecisionAndApproximateFrom(o);
			return true;
		} else if(o.denominatorIsTwo()) {
			if(b_imag) return false;
			clear(true);
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			if(o.numeratorIsOne()) {
				i_value->set(1, 1, 0);
			} else {
				mpz_t zrem;
				mpz_init(zrem);
				mpz_tdiv_r_ui(zrem, mpq_numref(o.internalRational()), 4);
				if(mpz_cmp_ui(zrem, 1) == 0 || mpz_cmp_si(zrem, -3) == 0) {
					i_value->set(1, 1, 0);
				} else {
					i_value->set(-1, 1, 0);
				}
			}
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}

	if(n_type == NUMBER_TYPE_RATIONAL && !o.isFloatingPoint()) {
		bool success = false;
		if(mpz_fits_slong_p(mpq_numref(o.internalRational())) != 0 && mpz_fits_ulong_p(mpq_denref(o.internalRational())) != 0) {
			long int i_pow = mpz_get_si(mpq_numref(o.internalRational()));
			unsigned long int i_root = mpz_get_ui(mpq_denref(o.internalRational()));
			size_t length1 = mpz_sizeinbase(mpq_numref(r_value), 10);
			size_t length2 = mpz_sizeinbase(mpq_denref(r_value), 10);
			if(length2 > length1) length1 = length2;
			if((i_root <= 2  || mpq_sgn(r_value) > 0) && ((!try_exact && i_root <= 3 && (long long int) labs(i_pow) * length1 < 1000) || (try_exact && (long long int) labs(i_pow) * length1 < 1000000LL && i_root < 1000000L))) {
				bool complex_result = false;
				if(i_root != 1) {
					mpq_t r_test;
					mpq_init(r_test);
					bool b_neg = i_pow < 0;
					if(b_neg) {
						mpq_inv(r_test, r_value);
						i_pow = -i_pow;
					} else {
						mpq_set(r_test, r_value);
					}
					if(mpz_cmp_ui(mpq_denref(r_test), 1) == 0) {
						if(i_pow != 1) mpz_pow_ui(mpq_numref(r_test), mpq_numref(r_test), (unsigned long int) i_pow);
						if(i_root % 2 == 0 && mpq_sgn(r_test) < 0) {
							if(b_imag) {mpq_clear(r_test); return false;}
							if(i_root == 2) {
								mpq_neg(r_test, r_test);
								success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root);
								complex_result = true;
							}
						} else {
							success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root);
						}
					} else {
						if(i_pow != 1) {
							mpz_pow_ui(mpq_numref(r_test), mpq_numref(r_test), (unsigned long int) i_pow);
							mpz_pow_ui(mpq_denref(r_test), mpq_denref(r_test), (unsigned long int) i_pow);
						}
						if(i_root % 2 == 0 && mpq_sgn(r_test) < 0) {
							if(b_imag) {mpq_clear(r_test); return false;}
							if(i_root == 2) {
								mpq_neg(r_test, r_test);
								success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) && mpz_root(mpq_denref(r_test), mpq_denref(r_test), i_root);
								complex_result = true;
							}
						} else {
							success = mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) && mpz_root(mpq_denref(r_test), mpq_denref(r_test), i_root);
						}
					}
					if(success) {
						if(complex_result) {
							if(b_neg) mpq_neg(r_test, r_test);
							if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
							i_value->setInternal(r_test, false, true);
							if(i_pow % 4 == 3) i_value->negate();
							mpq_set_ui(r_value, 0, 1);
						} else {
							mpq_set(r_value, r_test);
						}
					}
					mpq_clear(r_test);
				} else if(i_pow != 1) {
					if(i_pow < 0) {
						mpq_inv(r_value, r_value);
						i_pow = -i_pow;
					}
					if(i_pow != 1) {
						mpz_pow_ui(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) i_pow);
						if(mpz_cmp_ui(mpq_denref(r_value), 1) != 0) mpz_pow_ui(mpq_denref(r_value), mpq_denref(r_value), (unsigned long int) i_pow);
					}
					success = true;
				} else {
					success = true;
				}
			}
		}
		if(success) {
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	Number nr_bak(*this);
	
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value);

	bool try_complex = false;

	if(o.isFloatingPoint()) {
		int sgn_ol = mpfr_sgn(o.internalLowerFloat()), sgn_ou = mpfr_sgn(o.internalUpperFloat());
		if(sgn_ol < 0 && (sgn_l == 0 || sgn_l != sgn_u)) {
			//CALCULATOR->error(true, _("Division by zero."), NULL);
			set(nr_bak);
			return false;
		}
		if(sgn_l < 0) {
			try_complex = true;
		} else {
			if(!isInterval() && !o.isInterval()) {
				if(!CREATE_INTERVAL) {
					mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
					mpfr_pow(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
				}
			} else {
				mpfr_t f_value1, f_value2, f_value3, f_value4;
				mpfr_inits2(BIT_PRECISION, f_value1, f_value2, f_value3, f_value4, NULL);

				mpfr_pow(f_value1, fu_value, o.internalUpperFloat(), MPFR_RNDN);
				mpfr_pow(f_value2, fl_value, o.internalLowerFloat(), MPFR_RNDN);
				mpfr_pow(f_value3, fu_value, o.internalLowerFloat(), MPFR_RNDN);
				mpfr_pow(f_value4, fl_value, o.internalUpperFloat(), MPFR_RNDN);
				
				int i_upper = 1, i_lower = 1;
				
				if(mpfr_cmp(f_value2, f_value1) > 0) {
					if(mpfr_cmp(f_value3, f_value2) > 0) {
						if(mpfr_cmp(f_value4, f_value3) > 0) i_upper = 4;
						else i_upper = 3;
					} else if(mpfr_cmp(f_value4, f_value2) > 0) i_upper = 4;
					else i_upper = 2;
				} else if(mpfr_cmp(f_value3, f_value1) > 0) {
					if(mpfr_cmp(f_value4, f_value3) > 0) i_upper = 4;
					else i_upper = 3;
				} else if(mpfr_cmp(f_value4, f_value1) > 0) i_upper = 4;
				
				if(mpfr_cmp(f_value2, f_value1) < 0) {
					if(mpfr_cmp(f_value3, f_value2) < 0) {
						if(mpfr_cmp(f_value4, f_value3) < 0) i_lower = 4;
						else i_lower = 3;
					} else if(mpfr_cmp(f_value4, f_value2) < 0) i_lower = 4;
					else i_lower = 2;
				} else if(mpfr_cmp(f_value3, f_value1) < 0) {
					if(mpfr_cmp(f_value4, f_value3) < 0) i_lower = 4;
					else i_lower = 3;
				} else if(mpfr_cmp(f_value4, f_value1) < 0) i_lower = 4;

				switch(i_upper) {
					case 1: {mpfr_pow(f_value1, fu_value, o.internalUpperFloat(), MPFR_RNDU); break;}
					case 2: {mpfr_pow(f_value1, fl_value, o.internalLowerFloat(), MPFR_RNDU); break;}
					case 3: {mpfr_pow(f_value1, fu_value, o.internalLowerFloat(), MPFR_RNDU); break;}
					case 4: {mpfr_pow(f_value1, fl_value, o.internalUpperFloat(), MPFR_RNDU); break;}
				}
				switch(i_lower) {
					case 1: {mpfr_pow(fl_value, fu_value, o.internalUpperFloat(), MPFR_RNDD); break;}
					case 2: {mpfr_pow(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD); break;}
					case 3: {mpfr_pow(fl_value, fu_value, o.internalLowerFloat(), MPFR_RNDD); break;}
					case 4: {mpfr_pow(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD); break;}
				}
				mpfr_set(fu_value, f_value1, MPFR_RNDU);

				if(sgn_ou != sgn_ol) {
					if(mpfr_cmp_ui(fl_value, 1) > 0) mpfr_set_ui(fl_value, 1, MPFR_RNDD);
					else if(mpfr_cmp_ui(fu_value, 1) < 0) mpfr_set_ui(fu_value, 1, MPFR_RNDU);
				}

				mpfr_clears(f_value1, f_value2, f_value3, f_value4, NULL);
			}
		}
	} else {
		if(!o.isNonNegative() && (sgn_l == 0 || sgn_l != sgn_u)) {
			//CALCULATOR->error(true, _("Division by zero."), NULL);
			set(nr_bak);
			return false;
		}
		if(o.isInteger() && o.integerLength() < 1000000L) {
			if(!CREATE_INTERVAL && !isInterval()) {
				mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else if(o.isEven() && sgn_l < 0 && sgn_u >= 0) {
				if(mpfr_cmpabs(fu_value, fl_value) < 0) {
					mpfr_pow_z(fu_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDU);
				} else {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDU);
				}
				mpfr_set_ui(fl_value, 0, MPFR_RNDD);
			} else {
				bool b_reverse = o.isEven() && sgn_l < 0;
				if(o.isNegative()) {
					if(b_reverse) {
						b_reverse = false;
					} else {
						b_reverse = true;
					}
				}
				if(b_reverse) {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDD);
					mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDU);
					mpfr_swap(fu_value, fl_value);
				} else {
					mpfr_pow_z(fu_value, fu_value, mpq_numref(o.internalRational()), MPFR_RNDU);
					mpfr_pow_z(fl_value, fl_value, mpq_numref(o.internalRational()), MPFR_RNDD);
				}
			}
		} else {
			if(sgn_l < 0) {
				if(sgn_u < 0 && mpz_cmp_ui(mpq_denref(o.internalRational()), 2) == 0) {
					if(b_imag) {set(nr_bak); return false;}
					if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
					i_value->set(*this, false, true);
					if(!i_value->negate() || !i_value->raise(o)) {
						set(nr_bak);
						return false;
					}
					if(!o.numeratorIsOne()) {
						mpz_t zrem;
						mpz_init(zrem);
						mpz_tdiv_r_ui(zrem, mpq_numref(o.internalRational()), 4);
						if(mpz_cmp_ui(zrem, 1) != 0 && mpz_cmp_si(zrem, -3) != 0) i_value->negate();
					}
					clearReal();
					setPrecisionAndApproximateFrom(*i_value);
					return true;
				} else {
					try_complex = true;
				}
			} else {
				if(o.numeratorIsOne() && mpz_fits_ulong_p(mpq_denref(o.internalRational()))) {
					unsigned long int i_root = mpz_get_ui(mpq_denref(o.internalRational()));
					if(!CREATE_INTERVAL && !isInterval()) {
#if MPFR_VERSION_MAJOR < 4
						mpfr_root(fl_value, fl_value, i_root, MPFR_RNDN);
#else
						mpfr_rootn_ui(fl_value, fl_value, i_root, MPFR_RNDN);
#endif
						mpfr_set(fu_value, fl_value, MPFR_RNDN);
					} else {
#if MPFR_VERSION_MAJOR < 4
						mpfr_root(fu_value, fu_value, i_root, MPFR_RNDU);
						mpfr_root(fl_value, fl_value, i_root, MPFR_RNDD);
#else
						mpfr_rootn_ui(fu_value, fu_value, i_root, MPFR_RNDU);
						mpfr_rootn_ui(fl_value, fl_value, i_root, MPFR_RNDD);
#endif
					}
				} else if(!CREATE_INTERVAL && !isInterval()) {
					mpfr_t f_pow;
					mpfr_init2(f_pow, BIT_PRECISION);
					mpfr_set_q(f_pow, o.internalRational(), MPFR_RNDN);
					mpfr_pow(fl_value, fl_value, f_pow, MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
					mpfr_clears(f_pow, NULL);
				} else {
					mpfr_t f_pow_u, f_pow_l;
					mpfr_init2(f_pow_u, BIT_PRECISION);
					mpfr_init2(f_pow_l, BIT_PRECISION);
					mpfr_set_q(f_pow_u, o.internalRational(), MPFR_RNDU);
					mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDD);
					if(mpfr_equal_p(fu_value, fl_value) || mpfr_equal_p(f_pow_u, f_pow_l)) {
						if(mpfr_equal_p(fu_value, fl_value) && mpfr_equal_p(f_pow_u, f_pow_l)) {
							mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDU);
							mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD);
						} else if(mpfr_cmp_ui(fl_value, 1) < 0) {
							if(o.isNegative()) {
								mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDD);
								mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDU);
								mpfr_swap(fu_value, fl_value);
							} else {
								mpfr_pow(fu_value, fu_value, f_pow_l, MPFR_RNDU);
								mpfr_pow(fl_value, fl_value, f_pow_u, MPFR_RNDD);
							}
						} else {
							if(o.isNegative()) {
								mpfr_pow(fu_value, fu_value, f_pow_l, MPFR_RNDD);
								mpfr_pow(fl_value, fl_value, f_pow_u, MPFR_RNDU);
								mpfr_swap(fu_value, fl_value);
							} else {
								mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDU);
								mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD);
							}
						}
					} else {
						mpfr_t f_value1, f_value2, f_value3, f_value4;
						mpfr_inits2(BIT_PRECISION, f_value1, f_value2, f_value3, f_value4, NULL);

						mpfr_pow(f_value1, fu_value, f_pow_u, MPFR_RNDN);
						mpfr_pow(f_value2, fl_value, f_pow_l, MPFR_RNDN);
						mpfr_pow(f_value3, fu_value, f_pow_l, MPFR_RNDN);
						mpfr_pow(f_value4, fl_value, f_pow_u, MPFR_RNDN);
			
						int i_upper = 1, i_lower = 1;
						
						if(mpfr_cmp(f_value2, f_value1) > 0) {
							if(mpfr_cmp(f_value3, f_value2) > 0) {
								if(mpfr_cmp(f_value4, f_value3) > 0) i_upper = 4;
								else i_upper = 3;
							} else if(mpfr_cmp(f_value4, f_value2) > 0) i_upper = 4;
							else i_upper = 2;
						} else if(mpfr_cmp(f_value3, f_value1) > 0) {
							if(mpfr_cmp(f_value4, f_value3) > 0) i_upper = 4;
							else i_upper = 3;
						} else if(mpfr_cmp(f_value4, f_value1) > 0) i_upper = 4;
						
						if(mpfr_cmp(f_value2, f_value1) < 0) {
							if(mpfr_cmp(f_value3, f_value2) < 0) {
								if(mpfr_cmp(f_value4, f_value3) < 0) i_lower = 4;
								else i_lower = 3;
							} else if(mpfr_cmp(f_value4, f_value2) < 0) i_lower = 4;
							else i_lower = 2;
						} else if(mpfr_cmp(f_value3, f_value1) < 0) {
							if(mpfr_cmp(f_value4, f_value3) < 0) i_lower = 4;
							else i_lower = 3;
						} else if(mpfr_cmp(f_value4, f_value1) < 0) i_lower = 4;

						switch(i_upper) {
							case 1: {mpfr_pow(f_value1, fu_value, f_pow_u, MPFR_RNDU); break;}
							case 2: {mpfr_pow(f_value1, fl_value, f_pow_l, MPFR_RNDU); break;}
							case 3: {mpfr_pow(f_value1, fu_value, f_pow_l, MPFR_RNDU); break;}
							case 4: {mpfr_pow(f_value1, fl_value, f_pow_u, MPFR_RNDU); break;}
						}
						switch(i_lower) {
							case 1: {mpfr_pow(fl_value, fu_value, f_pow_u, MPFR_RNDD); break;}
							case 2: {mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD); break;}
							case 3: {mpfr_pow(fl_value, fu_value, f_pow_l, MPFR_RNDD); break;}
							case 4: {mpfr_pow(fl_value, fl_value, f_pow_u, MPFR_RNDD); break;}
						}
						mpfr_set(fu_value, f_value1, MPFR_RNDU);

						mpfr_clears(f_value1, f_value2, f_value3, f_value4, NULL);
					}
					mpfr_clears(f_pow_u, f_pow_l, NULL);
				}
			}
		}
	}
	if(try_complex) {
		set(nr_bak);
		if(sgn_l < 0 && sgn_u >= 0) {
			Number nr_neg(lowerEndPoint());
			if(!nr_neg.raise(o) || (b_imag && nr_neg.hasImaginaryPart())) return false;
			Number nr_pos(upperEndPoint());
			if(!nr_pos.raise(o)) return false;
			if(!nr_neg.setInterval(nr_zero, nr_neg) || !nr_pos.setInterval(nr_zero, nr_pos)) return false;
			if(!setInterval(nr_neg, nr_pos)) return false;
		} else {
			Number nbase, nexp(*this);
			nbase.e();
			if(!nexp.ln()) return false;
			if(!nexp.multiply(o)) return false;
			if(!nbase.raise(nexp, false)) return false;
			if(b_imag && nbase.hasImaginaryPart()) return false;
			set(nbase);
		}
		return true;
	}
	if(!testFloatResult(true) || (includesInfinity() && !nr_bak.includesInfinity() && !o.includesInfinity())) {
		set(nr_bak);
		return false;
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::sqrt() {
	if(hasImaginaryPart()) return raise(Number(1, 2, 0), true);
	if(isNegative()) {
		if(b_imag) return false;
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->set(*this, false, true);
		if(!i_value->negate() || !i_value->sqrt()) {
			i_value->clear();
			return false;
		}
		clearReal();
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_perfect_square_p(mpq_numref(r_value)) && mpz_perfect_square_p(mpq_denref(r_value))) {
			mpz_sqrt(mpq_numref(r_value), mpq_numref(r_value));
			mpz_sqrt(mpq_denref(r_value), mpq_denref(r_value));
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();

	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_sqrt(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) < 0) {
			if(b_imag) {
				set(nr_bak);
				return false;
			} else {
				if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
				if(mpfr_sgn(fu_value) > 0) {
					i_value->setInterval(lowerEndPoint(), nr_zero);
				} else {
					i_value->set(*this, false, true);
				}
				if(!i_value->abs() || !i_value->sqrt()) {set(nr_bak); return false;}
			}
			mpfr_sqrt(fu_value, fu_value, MPFR_RNDU);
			mpfr_set_zero(fl_value, 0);
		} else {
			mpfr_sqrt(fu_value, fu_value, MPFR_RNDU);
			mpfr_sqrt(fl_value, fl_value, MPFR_RNDD);
		}
	}
	
	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cbrt() {
	if(hasImaginaryPart()) return raise(Number(1, 3, 0), true);
	if(isOne() || isMinusOne() || isZero()) return true;
	Number nr_bak(*this);
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_root(mpq_numref(r_value), mpq_numref(r_value), 3) && mpz_root(mpq_denref(r_value), mpq_denref(r_value), 3)) {
			return true;
		}
		set(nr_bak);
	}
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_cbrt(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_cbrt(fu_value, fu_value, MPFR_RNDU);
		mpfr_cbrt(fl_value, fl_value, MPFR_RNDD);
	}
	
	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::root(const Number &o) {
	if(!o.isInteger() || !o.isPositive() || hasImaginaryPart() || (o.isEven() && !isNonNegative())) return false;
	if(isOne() || o.isOne() || isZero() || isPlusInfinity()) return true;
	if(o.isTwo()) return sqrt();
	/*if(o.isEven() && (!isReal() || isNegative())) {
		Number o_odd_factor(o);
		Number o_even_factor(1, 1, 0);
		while(o_odd_factor.isEven() && !o_odd_factor.isTwo()) {
			if(!o_odd_factor.multiply(nr_half) || !o_even_factor.multiply(2)) return false;
			if(CALCULATOR->aborted()) return false;
		}
		if(!o_even_factor.recip()) return false;
		Number nr_bak(*this);
		if(!root(o_odd_factor)) return false;
		if(!raise(o_even_factor)) {set(nr_bak); return false;}
		return true;
	}
	if(!isReal()) {
		if(!hasRealPart()) {
			Number nr_o(o);
			if(!nr_o.irem(4) || !i_value->root(o)) return false;
			if(!nr_o.isOne()) i_value->negate();
			return true;
		}
		return false;
	}*/
	if(isMinusOne()) return true;
	Number nr_bak(*this);
	if(!mpz_fits_ulong_p(mpq_numref(o.internalRational()))) {
		
		if(!setToFloatingPoint()) return false;
		
		Number o_inv(o);
		o_inv.recip();
		
		mpfr_t f_pow_u, f_pow_l;
		
		mpfr_init2(f_pow_u, BIT_PRECISION);
		mpfr_init2(f_pow_l, BIT_PRECISION);
		
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDN);
			int sgn_l = mpfr_sgn(fl_value);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDN);
			mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDN);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_set_q(f_pow_u, o.internalRational(), MPFR_RNDU);
			mpfr_set_q(f_pow_l, o.internalRational(), MPFR_RNDD);
			
			int sgn_l = mpfr_sgn(fl_value), sgn_u = mpfr_sgn(fu_value);
			
			if(sgn_u < 0) mpfr_neg(fu_value, fu_value, MPFR_RNDD);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDU);
			
			mpfr_pow(fu_value, fu_value, f_pow_u, MPFR_RNDU);
			mpfr_pow(fl_value, fl_value, f_pow_l, MPFR_RNDD);
			
			if(sgn_u < 0) mpfr_neg(fu_value, fu_value, MPFR_RNDU);
			if(sgn_l < 0) mpfr_neg(fl_value, fl_value, MPFR_RNDD);
		}
	
		mpfr_clears(f_pow_u, f_pow_l, NULL);
	} else {
		unsigned long int i_root = mpz_get_ui(mpq_numref(o.internalRational()));
		if(n_type == NUMBER_TYPE_RATIONAL) {
			if(mpz_root(mpq_numref(r_value), mpq_numref(r_value), i_root) && mpz_root(mpq_denref(r_value), mpq_denref(r_value), i_root)) {
				return true;
			}
			set(nr_bak);
			if(!setToFloatingPoint()) return false;
		}
		mpfr_clear_flags();
	
		if(!CREATE_INTERVAL && !isInterval()) {
#if MPFR_VERSION_MAJOR < 4
			mpfr_root(fl_value, fl_value, i_root, MPFR_RNDN);
#else
			mpfr_rootn_ui(fl_value, fl_value, i_root, MPFR_RNDN);
#endif
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
#if MPFR_VERSION_MAJOR < 4
			mpfr_root(fu_value, fu_value, i_root, MPFR_RNDU);
			mpfr_root(fl_value, fl_value, i_root, MPFR_RNDD);
#else
			mpfr_rootn_ui(fu_value, fu_value, i_root, MPFR_RNDU);
			mpfr_rootn_ui(fl_value, fl_value, i_root, MPFR_RNDD);
#endif
		}
	}
	
	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::allroots(const Number &o, vector<Number> &roots) {
	if(!o.isInteger() || !o.isPositive()) return false;
	if(isOne() || o.isOne() || isZero()) {
		roots.push_back(*this);
		return true;
	}
	if(o.isTwo()) {
		Number nr(*this);
		if(!nr.sqrt()) return false;
		roots.push_back(nr);
		if(!nr.negate()) return false;
		roots.push_back(nr);
		return true;
	}
	if(isInfinite()) return false;
	Number o_inv(o);
	if(!o_inv.recip()) return false;
	Number nr_arg;
	nr_arg.set(*this, false, true);
	if(!nr_arg.atan2(*i_value)) return false;
	Number nr_pi2;
	nr_pi2.pi();
	nr_pi2 *= 2;
	Number nr_i;
	Number nr_re;
	nr_re.set(*this, false, true);
	Number nr_im(*i_value);
	if(!nr_re.square() || !nr_im.square() || !nr_re.add(nr_im) || !nr_re.sqrt() || !nr_re.raise(o_inv)) return false;
	while(nr_i.isLessThan(o)) {
		if(CALCULATOR->aborted()) return false;
		Number nr(nr_pi2);
		if(!nr.multiply(nr_i) || !nr.add(nr_arg) || !nr.multiply(nr_one_i) || !nr.multiply(o_inv) || !nr.exp() || !nr.multiply(nr_re)) return false;
		roots.push_back(nr);
		nr_i++;
	}
	return true;
}
bool Number::exp10(const Number &o) {
	if(isZero()) return true;
	if(o.isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number ten(10, 1);
	if(!ten.raise(o)) {
		return false;
	}
	multiply(ten);
	return true;
}
bool Number::exp10() {
	if(isZero()) {
		set(1, 1);
		return true;
	}
	Number ten(10, 1);
	if(!ten.raise(*this)) {
		return false;
	}
	set(ten);
	return true;
}
bool Number::exp2(const Number &o) {
	if(isZero()) return true;
	if(o.isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number two(2, 1);
	if(!two.raise(o)) {
		return false;
	}
	multiply(two);
	return true;
}
bool Number::exp2() {
	if(isZero()) {
		set(1, 1);
		return true;
	}
	Number two(2, 1);
	if(!two.raise(*this)) {
		return false;
	}
	set(two);
	return true;
}
bool Number::square() {
	if(isInfinite()) {
		n_type = NUMBER_TYPE_PLUS_INFINITY;
		return true;
	}
	if(hasImaginaryPart()) {
		if(!hasRealPart()) {
			if(i_value->isFloatingPoint() && (CREATE_INTERVAL || i_value->isInterval())) {
				Number nr_bak(*this);
				if(!i_value->setToFloatingPoint()) return false;
				Number *i_copy = i_value;
				i_value = NULL;
				set(*i_copy, true);
				delete i_copy;
				if(!square() || !negate()) {set(nr_bak); return false;}
				return true;
			}
		} else if((n_type == NUMBER_TYPE_FLOAT || i_value->isFloatingPoint()) && (CREATE_INTERVAL || isInterval(false))) {
			Number nr_bak(*this);
			if(!setToFloatingPoint()) return false;
			if(!i_value->setToFloatingPoint()) {set(nr_bak); return false;}
			mpfr_t f_ru, f_rl, f_iu, f_il, f_tmp;
			mpfr_inits2(BIT_PRECISION, f_ru, f_rl, f_iu, f_il, f_tmp, NULL);
			if(mpfr_sgn(fl_value) < 0 && mpfr_sgn(fu_value) > 0) {
				mpfr_set_zero(f_rl, 0);
				if(mpfr_cmpabs(fl_value, fu_value) > 0) {
					mpfr_sqr(f_ru, fl_value, MPFR_RNDU);
				} else {
					mpfr_sqr(f_ru, fu_value, MPFR_RNDU);
				}
			} else {
				if(mpfr_cmpabs(fl_value, fu_value) > 0) {
					mpfr_sqr(f_ru, fl_value, MPFR_RNDU);
					mpfr_sqr(f_rl, fu_value, MPFR_RNDD);
				} else {
					mpfr_sqr(f_rl, fl_value, MPFR_RNDD);
					mpfr_sqr(f_ru, fu_value, MPFR_RNDU);
				}
			}
			if(mpfr_sgn(i_value->internalLowerFloat()) < 0 && mpfr_sgn(i_value->internalUpperFloat()) > 0) {
				if(mpfr_cmpabs(i_value->internalLowerFloat(), i_value->internalUpperFloat()) > 0) {
					mpfr_sqr(f_tmp, i_value->internalLowerFloat(), MPFR_RNDU);
					mpfr_sub(f_rl, f_rl, f_tmp, MPFR_RNDD);
				} else {
					mpfr_sqr(f_tmp, i_value->internalUpperFloat(), MPFR_RNDU);
					mpfr_sub(f_rl, f_rl, f_tmp, MPFR_RNDD);
				}
			} else if(mpfr_cmpabs(i_value->internalLowerFloat(), i_value->internalUpperFloat()) > 0) {
				mpfr_sqr(f_tmp, i_value->internalLowerFloat(), MPFR_RNDU);
				mpfr_sub(f_rl, f_rl, f_tmp, MPFR_RNDD);
				mpfr_sqr(f_tmp, i_value->internalUpperFloat(), MPFR_RNDD);
				mpfr_sub(f_ru, f_ru, f_tmp, MPFR_RNDU);
			} else {
				mpfr_sqr(f_tmp, i_value->internalUpperFloat(), MPFR_RNDU);
				mpfr_sub(f_rl, f_rl, f_tmp, MPFR_RNDD);
				mpfr_sqr(f_tmp, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_sub(f_ru, f_ru, f_tmp, MPFR_RNDU);
			}
			
			bool neg_rl = mpfr_sgn(fl_value) < 0, neg_ru = mpfr_sgn(fu_value) < 0, neg_il = mpfr_sgn(i_value->internalLowerFloat()) < 0, neg_iu = mpfr_sgn(i_value->internalUpperFloat()) < 0;
			if(neg_rl && !neg_ru && !neg_il) {
				mpfr_mul(f_il, fl_value, i_value->internalUpperFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fu_value, i_value->internalUpperFloat(), MPFR_RNDU);
			} else if(!neg_rl && neg_il && !neg_iu) {
				mpfr_mul(f_il, fu_value, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fu_value, i_value->internalUpperFloat(), MPFR_RNDU);
			} else if(neg_rl && neg_ru && !neg_il) {
				mpfr_mul(f_il, fl_value, i_value->internalUpperFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fu_value, i_value->internalLowerFloat(), MPFR_RNDU);
			} else if(!neg_rl && neg_il && neg_iu) {
				mpfr_mul(f_il, fu_value, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fl_value, i_value->internalUpperFloat(), MPFR_RNDU);
			} else if(neg_rl && neg_ru && neg_il && !neg_iu) {
				mpfr_mul(f_il, fl_value, i_value->internalUpperFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fl_value, i_value->internalLowerFloat(), MPFR_RNDU);
			} else if(neg_il && neg_iu && neg_rl && !neg_ru) {
				mpfr_mul(f_il, fu_value, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fl_value, i_value->internalLowerFloat(), MPFR_RNDU);
			} else if(neg_rl && !neg_ru && neg_il && !neg_iu) {
				mpfr_mul(f_il, fu_value, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_mul(f_tmp, fl_value, i_value->internalUpperFloat(), MPFR_RNDD);
				if(mpfr_cmp(f_tmp, f_il) < 0) mpfr_swap(f_il, f_tmp);
				mpfr_mul(f_iu, fu_value, i_value->internalUpperFloat(), MPFR_RNDU);
				mpfr_mul(f_tmp, fl_value, i_value->internalLowerFloat(), MPFR_RNDU);
				if(mpfr_cmp(f_tmp, f_iu) > 0) mpfr_swap(f_iu, f_tmp);
			} else if(neg_rl && neg_ru && neg_il && neg_iu) {
				mpfr_mul(f_iu, fl_value, i_value->internalLowerFloat(), MPFR_RNDU);
				mpfr_mul(f_il, fu_value, i_value->internalUpperFloat(), MPFR_RNDD);
			} else {
				mpfr_mul(f_il, fl_value, i_value->internalLowerFloat(), MPFR_RNDD);
				mpfr_mul(f_iu, fu_value, i_value->internalUpperFloat(), MPFR_RNDU);
			}
			mpfr_mul_ui(f_il, f_il, 2, MPFR_RNDD);
			mpfr_mul_ui(f_iu, f_iu, 2, MPFR_RNDU);
			mpfr_swap(f_rl, fl_value);
			mpfr_swap(f_ru, fu_value);
			mpfr_swap(f_il, i_value->internalLowerFloat());
			mpfr_swap(f_iu, i_value->internalUpperFloat());
			mpfr_clears(f_ru, f_rl, f_iu, f_il, f_tmp, NULL);
			if(!i_value->testFloatResult(true) || !testFloatResult(true)) {
				set(nr_bak);
				return false;
			}
			return true;
		}
		Number nr(*this);
		return multiply(nr);
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_mul(r_value, r_value, r_value);
	} else {
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_sqr(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else if(mpfr_sgn(fl_value) < 0) {
			if(mpfr_sgn(fu_value) < 0) {
				mpfr_sqr(fu_value, fu_value, MPFR_RNDD);
				mpfr_sqr(fl_value, fl_value, MPFR_RNDU);
				mpfr_swap(fu_value, fl_value);
			} else {
				if(mpfr_cmpabs(fu_value, fl_value) < 0) mpfr_sqr(fu_value, fl_value, MPFR_RNDU);
				else mpfr_sqr(fu_value, fu_value, MPFR_RNDU);
				mpfr_set_zero(fl_value, 0);
			}
		} else {
			mpfr_sqr(fu_value, fu_value, MPFR_RNDU);
			mpfr_sqr(fl_value, fl_value, MPFR_RNDD);
		}
		testFloatResult(true, 2);
	}
	return true;
}

bool Number::negate() {
	if(i_value) i_value->negate();
	switch(n_type) {
		case NUMBER_TYPE_PLUS_INFINITY: {
			n_type = NUMBER_TYPE_MINUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_MINUS_INFINITY: {
			n_type = NUMBER_TYPE_PLUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_RATIONAL: {
			mpq_neg(r_value, r_value);
			break;
		}
		case NUMBER_TYPE_FLOAT: {
			mpfr_clear_flags();
			if(!CREATE_INTERVAL && !isInterval()) {
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_neg(fu_value, fu_value, MPFR_RNDD);
				mpfr_neg(fl_value, fl_value, MPFR_RNDU);
				mpfr_swap(fu_value, fl_value);
			}
			testFloatResult(true, 2);
			break;
		}
		default: {break;}
	}
	return true;
}
void Number::setNegative(bool is_negative) {
	switch(n_type) {
		case NUMBER_TYPE_PLUS_INFINITY: {
			if(is_negative) n_type = NUMBER_TYPE_MINUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_MINUS_INFINITY: {
			if(!is_negative) n_type = NUMBER_TYPE_PLUS_INFINITY;
			break;
		}
		case NUMBER_TYPE_RATIONAL: {
			if(is_negative != (mpq_sgn(r_value) < 0)) mpq_neg(r_value, r_value);
			break;
		}
		case NUMBER_TYPE_FLOAT: {
			mpfr_clear_flags();
			if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
				if(is_negative) {
					mpfr_neg(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) < 0) mpfr_swap(fu_value, fl_value);
					mpfr_set_zero(fu_value, 0);
				} else {
					mpfr_abs(fl_value, fl_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fu_value, fl_value);
					mpfr_set_zero(fl_value, 0);
				}
			} else if(is_negative != (mpfr_sgn(fl_value) < 0)) {
				if(!CREATE_INTERVAL && !isInterval()) {
					mpfr_neg(fl_value, fl_value, MPFR_RNDN);
					mpfr_set(fu_value, fl_value, MPFR_RNDN);
				} else {
					mpfr_neg(fu_value, fu_value, MPFR_RNDD);
					mpfr_neg(fl_value, fl_value, MPFR_RNDU);
					mpfr_swap(fu_value, fl_value);
				}
				testFloatResult(true, 2);
			}
			break;
		}
		default: {break;}
	}
}
bool Number::abs() {
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number nr_bak(*this);
			if(!i_value->square()) return false;
			Number *i_v = i_value;
			i_value = NULL;
			if(!square() || !add(*i_v)) {
				set(nr_bak);
				return false;
			}
			i_v->clear();
			i_value = i_v;
			if(!raise(nr_half)) {
				set(nr_bak);
				return false;
			}
			return true;
		}
		set(*i_value, true, true);
		clearImaginary();
	}
	if(isInfinite()) {
		n_type = NUMBER_TYPE_PLUS_INFINITY;
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_abs(r_value, r_value);
	} else {
		if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
			mpfr_abs(fl_value, fl_value, MPFR_RNDU);
			if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fu_value, fl_value);
			mpfr_set_zero(fl_value, 0);
		} else if(mpfr_sgn(fl_value) < 0) {
			if(!CREATE_INTERVAL && !isInterval()) {
				mpfr_neg(fl_value, fl_value, MPFR_RNDN);
				mpfr_set(fu_value, fl_value, MPFR_RNDN);
			} else {
				mpfr_neg(fu_value, fu_value, MPFR_RNDD);
				mpfr_neg(fl_value, fl_value, MPFR_RNDU);
				mpfr_swap(fu_value, fl_value);
			}
			testFloatResult(true, 2);
		}
	}
	return true;
}
bool Number::signum() {
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number nabs(*this);
			if(!nabs.abs() || !nabs.recip()) return false;
			return multiply(nabs);
		}
		return i_value->signum();
	}
	if(isPositive()) {set(1, 1); return true;}
	if(isNegative()) {set(-1, 1); return true;}
	return false;
}
bool Number::round(bool halfway_to_even) {
	if(includesInfinity() || hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_t i_rem;
			mpz_init(i_rem);
			mpz_mul_ui(mpq_numref(r_value), mpq_numref(r_value), 2);
			mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_mul_ui(mpq_denref(r_value), mpq_denref(r_value), 2);
			mpz_fdiv_qr(mpq_numref(r_value), i_rem, mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
			if(mpz_sgn(i_rem) == 0 && (!halfway_to_even || mpz_odd_p(mpq_numref(r_value)))) {
				if(halfway_to_even) mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
				else if(mpz_sgn(mpq_numref(r_value)) <= 0) mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			}
			mpz_clear(i_rem);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		intervalToMidValue();
		if(isRational()) return true;
		if(!halfway_to_even) mpfr_rint_round(fl_value, fl_value, MPFR_RNDN);
		mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDN);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::floor() {
	if(isInfinite() || hasImaginaryPart()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_fdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		if(mpfr_inf_p(fl_value)) return false;
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDD);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::ceil() {
	if(isInfinite() || hasImaginaryPart()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_cdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		if(mpfr_inf_p(fu_value)) return false;
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), fu_value, MPFR_RNDU);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::trunc() {
	if(isInfinite() || hasImaginaryPart()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_tdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		if(mpfr_inf_p(fl_value) && mpfr_inf_p(fu_value)) return false;
		mpz_set_ui(mpq_denref(r_value), 1);
		if(mpfr_sgn(fu_value) <= 0) mpfr_get_z(mpq_numref(r_value), fu_value, MPFR_RNDU);
		else if(mpfr_sgn(fl_value) >= 0) mpfr_get_z(mpq_numref(r_value), fl_value, MPFR_RNDD);
		else mpz_set_ui(mpq_numref(r_value), 0);
		n_type = NUMBER_TYPE_RATIONAL;
		mpfr_clears(fl_value, fu_value, NULL);
	}
	return true;
}
bool Number::round(const Number &o, bool halfway_to_even) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && round();
	}
	if(hasImaginaryPart()) return false;
	if(o.hasImaginaryPart()) return false;
	return divide(o) && round(halfway_to_even);
}
bool Number::floor(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && floor();
	}
	if(hasImaginaryPart()) return false;
	if(o.hasImaginaryPart()) return false;
	return divide(o) && floor();
}
bool Number::ceil(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && ceil();
	}
	if(hasImaginaryPart()) return false;
	if(o.hasImaginaryPart()) return false;
	return divide(o) && ceil();
}
bool Number::trunc(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && trunc();
	}
	if(hasImaginaryPart()) return false;
	if(o.hasImaginaryPart()) return false;
	return divide(o) && trunc();
}
bool Number::mod(const Number &o) {
	if(includesInfinity() || o.includesInfinity()) return false;
	if(hasImaginaryPart() || o.hasImaginaryPart()) return false;
	if(o.isZero()) return false;
	if(isRational() && o.isRational()) {
		if(isInteger() && o.isInteger()) {
			mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		} else {
			mpq_div(r_value, r_value, o.internalRational());
			mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpq_mul(r_value, r_value, o.internalRational());
		}
	} else {
		// TODO: Interval too wide when o is interval
		if(!divide(o) || !frac()) return false;
		if(isNegative()) {
			(*this)++;
			testFloatResult(true, 2);
		}
		return multiply(o);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::frac() {
	if(includesInfinity() || hasImaginaryPart()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(isInteger()) {
			clear();
		} else {
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
		}
	} else {
		mpfr_clear_flags();
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_frac(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else if(!isInterval()) {
			mpfr_frac(fl_value, fl_value, MPFR_RNDD);
			mpfr_frac(fu_value, fu_value, MPFR_RNDU);
		} else {
			mpfr_t testf, testu;
			mpfr_inits2(mpfr_get_prec(fl_value), testf, testu, NULL);
			mpfr_trunc(testf, fl_value);
			mpfr_trunc(testu, fu_value);
			if(!mpfr_equal_p(testf, testu)) {
				mpfr_set_zero(fl_value, 0);
				mpfr_set_ui(fu_value, 1, MPFR_RNDU);
			} else {
				mpfr_frac(testf, fl_value, MPFR_RNDU);
				mpfr_frac(testu, fu_value, MPFR_RNDU);
				if(mpfr_cmp(testf, testu) > 0) {
					mpfr_frac(fu_value, fl_value, MPFR_RNDU);
					mpfr_frac(fl_value, fu_value, MPFR_RNDD);
				} else {
					mpfr_frac(fl_value, fl_value, MPFR_RNDD);
					mpfr_frac(fu_value, fu_value, MPFR_RNDU);
				}
			}
			mpfr_clears(testf, testu, NULL);
		}
		testFloatResult(true, 2);
	}
	return true;
}
bool Number::rem(const Number &o) {
	if(includesInfinity() || o.includesInfinity()) return false;
	if(hasImaginaryPart() || o.hasImaginaryPart()) return false;
	if(o.isZero()) return false;
	if(isRational() && o.isRational()) {
		if(isInteger() && o.isInteger()) {
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		} else {
			mpq_div(r_value, r_value, o.internalRational());
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpq_mul(r_value, r_value, o.internalRational());
		}
	} else {
		// TODO: Interval too wide when o is interval
		return divide(o) && frac() && multiply(o);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::smod(const Number &o) {
	if(!isInteger() || !o.isInteger()) return false;
	mpz_t b2;
	mpz_init(b2);
	mpz_div_ui(b2, mpq_numref(o.internalRational()), 2);
	mpz_sub_ui(b2, b2, 1);
	mpz_add(mpq_numref(r_value), mpq_numref(r_value), b2);
	mpz_fdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	mpz_sub(mpq_numref(r_value), mpq_numref(r_value), b2);
	mpz_clear(b2);
	setPrecisionAndApproximateFrom(o);
	return true;
}	
bool Number::irem(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::irem(const Number &o, Number &q) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	q.set(1, 0);
	mpz_tdiv_qr(mpq_numref(q.internalRational()), mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::iquo(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	mpz_tdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::iquo(unsigned long int i) {
	if(i == 0) return false;
	if(!isInteger()) return false;
	mpz_tdiv_q_ui(mpq_numref(r_value), mpq_numref(r_value), i);
	return true;
}
bool Number::iquo(const Number &o, Number &r) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	r.set(1, 0);
	mpz_tdiv_qr(mpq_numref(r_value), mpq_numref(r.internalRational()), mpq_numref(r_value), mpq_numref(o.internalRational()));
	return true;
}
bool Number::isIntegerDivisible(const Number &o) const {
	if(!isInteger() || !o.isInteger()) return false;
	return mpz_divisible_p(mpq_numref(r_value), mpq_numref(o.internalRational()));
}
bool Number::isqrt() {
	if(isInteger()) {
		if(mpz_sgn(mpq_numref(r_value)) < 0) return false;
		mpz_sqrt(mpq_numref(r_value), mpq_numref(r_value));
		return true;
	}
	return false;
}
bool Number::isPerfectSquare() const {
	if(isInteger()) {
		if(mpz_sgn(mpq_numref(r_value)) < 0) return false;
		return mpz_perfect_square_p(mpq_numref(r_value)) != 0;
	}
	return false;
}

int Number::getBoolean() const {
	if(isNonZero()) {
		return 1;
	} else if(isZero()) {
		return 0;
	}
	return -1;
}
void Number::toBoolean() {
	setTrue(isNonZero());
}
void Number::setTrue(bool is_true) {
	if(is_true) {
		set(1, 0);
	} else {
		clear();
	}
}
void Number::setFalse() {
	setTrue(false);
}
void Number::setLogicalNot() {
	setTrue(!isNonZero());
}

void Number::e(bool use_cached_number) {
	if(use_cached_number) {
		if(nr_e.isZero() || CREATE_INTERVAL != nr_e.isInterval() || mpfr_get_prec(nr_e.internalLowerFloat()) < BIT_PRECISION) {
			nr_e.e(false);
		}
		set(nr_e);
	} else {
		if(n_type != NUMBER_TYPE_FLOAT) {
			mpfr_init2(fu_value, BIT_PRECISION); 
			mpfr_init2(fl_value, BIT_PRECISION); 
			mpq_set_ui(r_value, 0, 1);
		} else {
			if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
			if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
		}
		n_type = NUMBER_TYPE_FLOAT;
		if(!CREATE_INTERVAL) {
			mpfr_set_ui(fl_value, 1, MPFR_RNDN);
			mpfr_exp(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
			i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
		} else {
			mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			mpfr_set_ui(fu_value, 1, MPFR_RNDU);
			mpfr_exp(fu_value, fu_value, MPFR_RNDU);
			mpfr_exp(fl_value, fl_value, MPFR_RNDD);
		}
	}
	b_approx = true;
}
void Number::pi() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CREATE_INTERVAL) {
		mpfr_const_pi(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_pi(fu_value, MPFR_RNDU);
		mpfr_const_pi(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
void Number::catalan() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CREATE_INTERVAL) {
		mpfr_const_catalan(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_catalan(fu_value, MPFR_RNDU);
		mpfr_const_catalan(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
void Number::euler() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_init2(fu_value, BIT_PRECISION); 
		mpfr_init2(fl_value, BIT_PRECISION); 
		mpq_set_ui(r_value, 0, 1);
	} else {
		if(mpfr_get_prec(fu_value) < BIT_PRECISION) mpfr_set_prec(fu_value, BIT_PRECISION);
		if(mpfr_get_prec(fl_value) < BIT_PRECISION) mpfr_set_prec(fl_value, BIT_PRECISION);
	}
	n_type = NUMBER_TYPE_FLOAT;
	if(!CREATE_INTERVAL) {
		mpfr_const_euler(fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		i_precision = FROM_BIT_PRECISION(BIT_PRECISION);
	} else {
		mpfr_const_euler(fu_value, MPFR_RNDU);
		mpfr_const_euler(fl_value, MPFR_RNDD);
	}
	b_approx = true;
}
bool Number::zeta() {
	if(!isInteger() || !isPositive() || isOne()) {
		return false;
	}
	bool overflow = false;
	long int i = lintValue(&overflow);
	if(overflow) {
		CALCULATOR->error(true, _("Cannot handle an argument (s) that large for Riemann Zeta."), NULL);
		return false;
	}
	
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_zeta_ui(fl_value, (unsigned long int) i, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_zeta_ui(fu_value, (unsigned long int) i, MPFR_RNDU);
		mpfr_zeta_ui(fl_value, (unsigned long int) i, MPFR_RNDD);
	}
	
	mpq_set_ui(r_value, 0, 1);
	n_type = NUMBER_TYPE_FLOAT;
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	
	return true;
}
bool Number::gamma() {
	if(isPlusInfinity()) return true;
	if(!isReal()) return false;
	if(!isNonZero()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_gamma(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) > 0) {
			if(mpfr_cmp_d(fl_value, 1.5) < 0) {
				mpfr_t f_gamma_minx;
				mpfr_init2(f_gamma_minx, BIT_PRECISION); 
				mpfr_set_str(f_gamma_minx, "1.46163214496836234126265954232572132846819620400644635129598840859878644035380181024307499273372559", 10, MPFR_RNDN);
				if(mpfr_cmp(fl_value, f_gamma_minx) < 0) {
					if(mpfr_cmp(fu_value, f_gamma_minx) < 0) {
						mpfr_gamma(fu_value, fu_value, MPFR_RNDD);
						mpfr_gamma(fl_value, fl_value, MPFR_RNDU);
						mpfr_swap(fl_value, fu_value);
					} else {
						mpfr_gamma(fu_value, fu_value, MPFR_RNDU);
						mpfr_gamma(fl_value, fl_value, MPFR_RNDU);
						if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
						mpfr_set_str(fl_value, "0.88560319441088870027881590058258873320795153366990344887120016587513622741739634666479828021420359", 10, MPFR_RNDD);
					}
				} else {
					mpfr_gamma(fu_value, fu_value, MPFR_RNDU);
					mpfr_gamma(fl_value, fl_value, MPFR_RNDD);
				}
				mpfr_clear(f_gamma_minx);
			} else {
				mpfr_gamma(fu_value, fu_value, MPFR_RNDU);
				mpfr_gamma(fl_value, fl_value, MPFR_RNDD);
			}
		} else if(mpfr_sgn(fu_value) >= 0) {
			set(nr_bak);
			return false;
		} else {
			mpfr_t fu_test, fl_test;
			mpfr_init2(fu_test, BIT_PRECISION); 
			mpfr_init2(fl_test, BIT_PRECISION); 
			mpfr_floor(fu_test, fu_value);
			mpfr_floor(fl_test, fl_value);
			if(!mpfr_equal_p(fu_test, fl_test) || mpfr_equal_p(fl_test, fl_value)) {set(nr_bak); return false;}
			mpfr_gamma(fu_value, fu_value, MPFR_RNDN);
			mpfr_gamma(fl_value, fl_value, MPFR_RNDN);
			if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
			if(nr_bak.isInterval() && nr_bak.precision(1) <= PRECISION + 20) CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_gamma->name().c_str(), NULL);
			mpfr_clears(fu_test, fl_test, NULL);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::digamma() {
	if(!isReal()) return false;
	if(!isNonZero()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_digamma(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) > 0) {
			mpfr_digamma(fu_value, fu_value, MPFR_RNDU);
			mpfr_digamma(fl_value, fl_value, MPFR_RNDD);
		} else if(mpfr_sgn(fu_value) >= 0) {
			set(nr_bak);
			return false;
		} else {
			mpfr_t fu_test, fl_test;
			mpfr_init2(fu_test, BIT_PRECISION); 
			mpfr_init2(fl_test, BIT_PRECISION); 
			mpfr_floor(fu_test, fu_value);
			mpfr_floor(fl_test, fl_value);
			if(!mpfr_equal_p(fu_test, fl_test) || mpfr_equal_p(fl_test, fl_value)) {set(nr_bak); return false;}
			mpfr_digamma(fu_value, fu_value, MPFR_RNDU);
			mpfr_digamma(fl_value, fl_value, MPFR_RNDD);
			mpfr_clears(fu_test, fl_test, NULL);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::erf() {
	if(hasImaginaryPart()) return false;
	if(isPlusInfinity()) {set(1, 1, 0, true); return true;}
	if(isMinusInfinity()) {set(-1, 1, 0, true); return true;}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_erf(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_erf(fu_value, fu_value, MPFR_RNDU);
		mpfr_erf(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::erfc() {
	if(hasImaginaryPart()) return false;
	if(isPlusInfinity()) {clear(true); return true;}
	if(isMinusInfinity()) {set(2, 1, 0, true); return true;}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_erfc(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_erfc(fu_value, fu_value, MPFR_RNDD);
		mpfr_erfc(fl_value, fl_value, MPFR_RNDU);
		mpfr_swap(fu_value, fl_value);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::airy() {
	if(!isReal()) return false;
	if(!isLessThanOrEqualTo(500) || !isGreaterThanOrEqualTo(-500)) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_ai(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else if(mpfr_cmp_si(fl_value, -1) >= 0) {
		mpfr_ai(fl_value, fl_value, MPFR_RNDU);
		mpfr_ai(fu_value, fu_value, MPFR_RNDD);
		mpfr_swap(fl_value, fu_value);
	} else {
		mpfr_ai(fl_value, fl_value, MPFR_RNDN);
		mpfr_ai(fu_value, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		if(nr_bak.isInterval() && nr_bak.precision(1) <= PRECISION + 20) CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_airy->name().c_str(), NULL);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::besselj(const Number &o) {
	if(hasImaginaryPart() || !o.isInteger()) return false;
	if(isZero()) return true;
	if(isInfinite()) {
		clear(true);
		return true;
	}
	if(!mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(o.internalRational()));
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_jn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_jn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_jn(fu_value, n, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		if(nr_bak.isInterval() && nr_bak.precision(1) <= PRECISION + 20) CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_besselj->name().c_str(), NULL);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::bessely(const Number &o) {
	if(hasImaginaryPart() || !isNonNegative() || !o.isInteger() || isZero()) return false;
	if(isPlusInfinity()) {
		clear(true);
		return true;
	}
	if(isMinusInfinity()) return false;
	if(!mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(o.internalRational()));
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 2000000L) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_yn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_yn(fl_value, n, fl_value, MPFR_RNDN);
		mpfr_yn(fu_value, n, fu_value, MPFR_RNDN);
		if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
		if(nr_bak.isInterval() && nr_bak.precision(1) <= PRECISION + 20) CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_bessely->name().c_str(), NULL);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}

bool Number::sin() {
	if(includesInfinity()) {
		if(!hasImaginaryPart() && isInterval() && (mpfr_sgn(fl_value) != mpfr_sgn(fu_value) || !mpfr_inf_p(fl_value) || !mpfr_inf_p(fu_value))) {
			setInterval(nr_minus_one, nr_one, true);
			return true;
		}
		return false;
	}
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.sin() || !t1b.cosh() || !t2a.cos() || !t2b.sinh() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->sinh()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > BIT_PRECISION || mpfr_get_exp(fu_value) > BIT_PRECISION) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpz_t f_int;
			mpz_init(f_int);
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			mpfr_get_z(f_int, f_quo, MPFR_RNDD);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_abs(f_quo, f_quo, MPFR_RNDN);
			mpfr_mul_ui(f_quo, f_quo, 2, MPFR_RNDN);
			mpfr_sub_ui(f_quo, f_quo, 1, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				if(mpz_odd_p(f_int)) set(-1, 1, 0, true);
				else set(1, 1, 0, true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}		
			mpfr_clears(f_pi, f_quo, NULL);
			mpz_clear(f_int);
		}
		mpfr_sin(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_t fl_int, fu_int, f_diff;
		mpz_inits(fl_int, fu_int, f_diff, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		mpfr_div(fl_quo, fl_value, fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, fl_pi, MPFR_RNDU);
		mpfr_sub_q(fl_quo, fl_quo, nr_half.internalRational(), MPFR_RNDD);
		mpfr_sub_q(fu_quo, fu_quo, nr_half.internalRational(), MPFR_RNDU);
		mpfr_get_z(fl_int, fl_quo, MPFR_RNDD);
		mpfr_get_z(fu_int, fu_quo, MPFR_RNDD);
		mpfr_sub_z(fl_quo, fl_quo, fl_int, MPFR_RNDD);
		mpfr_sub_z(fu_quo, fu_quo, fu_int, MPFR_RNDU);
		if(mpz_cmp(fl_int, fu_int) != 0) {
			mpz_sub(f_diff, fu_int, fl_int);
			if(mpz_cmp_ui(f_diff, 2) >= 0) {
				mpfr_set_si(fl_value, -1, MPFR_RNDD);
				mpfr_set_si(fu_value, 1, MPFR_RNDU);
			} else {
				if(mpz_even_p(fl_int)) {
					mpfr_sin(fl_value, fl_value, MPFR_RNDU);
					mpfr_sin(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fl_value, -1, MPFR_RNDD);
				} else {
					mpfr_sin(fl_value, fl_value, MPFR_RNDD);
					mpfr_sin(fu_value, fu_value, MPFR_RNDD);
					if(mpfr_cmp(fu_value, fl_value) < 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fu_value, 1, MPFR_RNDU);
				}
			}
		} else {
			if(mpz_even_p(fl_int)) {
				mpfr_sin(fl_value, fl_value, MPFR_RNDU);
				mpfr_sin(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			} else {
				mpfr_sin(fl_value, fl_value, MPFR_RNDD);
				mpfr_sin(fu_value, fu_value, MPFR_RNDU);
			}
		}	
		mpfr_clears(fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_clears(fl_int, fu_int, f_diff, NULL);
	}
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::asin() {
	if(includesInfinity()) return false;
	if(isZero()) return true;
	if(isOne()) {
		pi();
		divide(2);
		return true;
	}
	if(isMinusOne()) {
		pi();
		divide(-2);
		return true;
	}
	if(hasImaginaryPart() || !isFraction()) {
		if(b_imag) return false;
		if(hasImaginaryPart() && !hasRealPart()) {
			Number nri(*i_value);
			if(!nri.asinh() || !nri.multiply(nr_one_i)) return false;
			set(nri, true);
			return true;
		}
		if(isInterval(false)) {
			Number nr1(lowerEndPoint(true));
			Number nr2(upperEndPoint(true));
			if(!nr1.asin() || !nr2.asin()) return false;
			if(!hasImaginaryPart()) {
				if(!setInterval(nr1, nr2, true)) return false;
				return true;
			}
			Number nr;
			if(!nr.setInterval(nr1, nr2, true)) return false;
			if(isInterval(true) && imaginaryPartIsInterval()) {
				Number nr3(lowerEndPoint(false));
				Number nr4(upperEndPoint(false));
				nr3.setImaginaryPart(i_value->upperEndPoint());
				nr4.setImaginaryPart(i_value->lowerEndPoint());
				if(!nr3.asin() || !nr4.asin()) return false;
				if(!nr.setInterval(nr, nr3, true)) return false;
				if(!nr.setInterval(nr, nr4, true)) return false;
			}
			if(hasRealPart() && !realPartIsNonZero()) {
				nr1 = lowerEndPoint(true);
				nr2 = upperEndPoint(true);
				nr1.clearReal();
				nr2.clearReal();
				if(!nr1.asin() || !nr2.asin()) return false;
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			if(hasImaginaryPart() && !imaginaryPartIsNonZero()) {
				nr1 = lowerEndPoint(false);
				nr2 = upperEndPoint(false);
				if(!nr1.asin() || !nr2.asin()) return false;
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			set(nr, true);
			return true;
		}
		Number z_sqln(*this);
		Number i_z(*this);
		bool b_neg = false;
		if(hasImaginaryPart()) {
			b_neg = (realPartIsNegative() && !imaginaryPartIsNegative()) || (realPartIsPositive() && imaginaryPartIsPositive());
		} else {
			b_neg = isNegative();
		}
		if(b_neg && (!z_sqln.negate() || !i_z.negate())) return false;
		if(!i_z.multiply(nr_one_i)) return false;
		if(!z_sqln.square() || !z_sqln.negate() || !z_sqln.add(1) || !z_sqln.raise(nr_half) || !z_sqln.add(i_z) || !z_sqln.ln() || !z_sqln.multiply(nr_minus_i)) return false;
		if(b_neg && !z_sqln.negate()) return false;
		if(hasImaginaryPart() && z_sqln.isInterval(false) && z_sqln.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
		set(z_sqln);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_asin(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_asin(fl_value, fl_value, MPFR_RNDD);
		mpfr_asin(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::sinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.sinh() || !t1b.cos() || !t2a.cosh() || !t2b.sin() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		} else {
			if(!i_value->sin()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 28 || mpfr_get_exp(fu_value) > 28) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_sinh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_sinh(fl_value, fl_value, MPFR_RNDD);
		mpfr_sinh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::asinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		 if(!hasRealPart()) {
			Number inr(*i_value);
			if(!inr.asin() || !inr.multiply(nr_one_i)) return false;
			set(inr, true);
			return true;
		}
		if(isInterval(false)) {
			Number nr1(lowerEndPoint(true));
			Number nr2(upperEndPoint(true));
			if(!nr1.asinh() || !nr2.asinh()) return false;
			Number nr;
			if(!nr.setInterval(nr1, nr2, true)) return false;
			if(isInterval(true) && imaginaryPartIsInterval()) {
				Number nr3(lowerEndPoint(false));
				Number nr4(upperEndPoint(false));
				nr3.setImaginaryPart(i_value->upperEndPoint());
				nr4.setImaginaryPart(i_value->lowerEndPoint());
				if(!nr3.asinh() || !nr4.asinh()) return false;
				if(!nr.setInterval(nr, nr3, true)) return false;
				if(!nr.setInterval(nr, nr4, true)) return false;
			}
			if(hasRealPart() && !realPartIsNonZero()) {
				nr1 = lowerEndPoint(true);
				nr2 = upperEndPoint(true);
				nr1.clearReal();
				nr2.clearReal();
				if(!nr1.asinh() || !nr2.asinh()) return false;
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			if(hasImaginaryPart() && !imaginaryPartIsNonZero()) {
				nr1 = lowerEndPoint(false);
				nr2 = upperEndPoint(false);
				if(!nr1.asinh() || !nr2.asinh()) return false;
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			set(nr, true);
			return true;
		}
		Number z_sqln(*this);
		if(!z_sqln.square() || !z_sqln.add(1) || !z_sqln.raise(nr_half) || !z_sqln.add(*this)) return false;
		//If zero, it means that the precision is too low (since infinity is not the correct value). Happens with number less than -(10^1000)i
		if(z_sqln.isZero()) return false;
		if(!z_sqln.ln()) return false;
		if(hasImaginaryPart() && z_sqln.isInterval(false) && z_sqln.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
		set(z_sqln);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_asinh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_asinh(fl_value, fl_value, MPFR_RNDD);
		mpfr_asinh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cos() {
	if(includesInfinity()) {
		if(!hasImaginaryPart() && isInterval() && (mpfr_sgn(fl_value) != mpfr_sgn(fu_value) || !mpfr_inf_p(fl_value) || !mpfr_inf_p(fu_value))) {
			setInterval(nr_minus_one, nr_one, true);
			return true;
		}
		return false;
	}
	if(isZero()) {
		set(1, 1, 0, true);
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.cos() || !t1b.cosh() || !t2a.sin() || !t2b.sinh() || !t1a.multiply(t1b) || !t2a.multiply(t2b) || !t2a.negate()) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			testComplex(this, i_value);
			return true;
		} else {
			if(!i_value->cosh()) return false;
			set(*i_value, true, true);
			i_value->clear();
			return true;
		}
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > BIT_PRECISION || mpfr_get_exp(fu_value) > BIT_PRECISION) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpz_t f_int;
			mpz_init(f_int);
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			// ?: was MPFR_RNDF
			mpfr_get_z(f_int, f_quo, MPFR_RNDD);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				if(mpz_odd_p(f_int)) set(-1, 1, 0, true);
				else set(1, 1, 0, true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_abs(f_quo, f_quo, MPFR_RNDN);
			mpfr_mul_ui(f_quo, f_quo, 2, MPFR_RNDN);
			mpfr_sub_ui(f_quo, f_quo, 1, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				mpz_clear(f_int);
				return true;
			}
			mpfr_clears(f_pi, f_quo, NULL);
			mpz_clear(f_int);
		}
		mpfr_cos(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_t fl_int, fu_int, f_diff;
		mpz_inits(fl_int, fu_int, f_diff, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		mpfr_div(fl_quo, fl_value, fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, fl_pi, MPFR_RNDU);
		mpfr_get_z(fl_int, fl_quo, MPFR_RNDD);
		mpfr_get_z(fu_int, fu_quo, MPFR_RNDD);
		mpfr_sub_z(fl_quo, fl_quo, fl_int, MPFR_RNDD);
		mpfr_sub_z(fu_quo, fu_quo, fu_int, MPFR_RNDU);
		if(mpz_cmp(fl_int, fu_int) != 0) {
			mpz_sub(f_diff, fu_int, fl_int);
			if(mpz_cmp_ui(f_diff, 2) >= 0) {
				mpfr_set_si(fl_value, -1, MPFR_RNDD);
				mpfr_set_si(fu_value, 1, MPFR_RNDU);
			} else {
				if(mpz_even_p(fl_int)) {
					mpfr_cos(fl_value, fl_value, MPFR_RNDU);
					mpfr_cos(fu_value, fu_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fl_value, -1, MPFR_RNDD);
				} else {
					mpfr_cos(fl_value, fl_value, MPFR_RNDD);
					mpfr_cos(fu_value, fu_value, MPFR_RNDD);
					if(mpfr_cmp(fu_value, fl_value) < 0) mpfr_swap(fl_value, fu_value);
					mpfr_set_si(fu_value, 1, MPFR_RNDU);
				}
			}
		} else {
			if(mpz_even_p(fl_int)) {
				mpfr_cos(fl_value, fl_value, MPFR_RNDU);
				mpfr_cos(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			} else {
				mpfr_cos(fl_value, fl_value, MPFR_RNDD);
				mpfr_cos(fu_value, fu_value, MPFR_RNDU);
			}
		}	
		mpfr_clears(fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpz_clears(fl_int, fu_int, f_diff, NULL);
	}
	
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}	
bool Number::acos() {
	if(includesInfinity()) return false;
	if(isOne()) {
		clear(true);
		return true;
	}
	if(isZero()) {
		pi();
		divide(2);
		return true;
	}
	if(isMinusOne()) {
		pi();
		return true;
	}
	if(hasImaginaryPart() || !isFraction()) {
		if(b_imag) return false;
		//acos(x)=(pi-2*asin(x))/2
		Number nr(*this);
		Number nr_pi;
		nr_pi.pi();
		if(!nr.asin() || !nr.multiply(2) || !nr.negate() || !nr.add(nr_pi) || !nr.multiply(nr_half)) return false;
		set(nr);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();

	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_acos(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_acos(fl_value, fl_value, MPFR_RNDU);
		mpfr_acos(fu_value, fu_value, MPFR_RNDD);
		mpfr_swap(fl_value, fu_value);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::cosh() {
	if(isInfinite()) {
		setPlusInfinity();
		return true;
	}
	if(isZero()) {
		set(1, 1, 0, true);
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.cosh() || !t1b.cos() || !t2a.sinh() || !t2b.sin() || !t1a.multiply(t1b) || !t2a.multiply(t2b)) return false;
			if(!t1a.isReal() || !t2a.isReal()) return false;
			set(t1a, true, true);
			i_value->set(t2a, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			testComplex(this, i_value);
			return true;
		} else {
			if(!i_value->cos()) return false;
			set(*i_value, true, true);
			i_value->clear();
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	if(mpfr_get_exp(fl_value) > 28 || mpfr_get_exp(fu_value) > 28) {
		set(nr_bak);
		return false;
	}
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_cosh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) < 0) {
			if(mpfr_sgn(fu_value) == 0) {
				mpfr_cosh(fu_value, fl_value, MPFR_RNDU);
				mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			} else if(mpfr_sgn(fu_value) > 0) {
				mpfr_cosh(fl_value, fl_value, MPFR_RNDU);
				mpfr_cosh(fu_value, fu_value, MPFR_RNDD);
				if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
				mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			} else {
				mpfr_cosh(fl_value, fl_value, MPFR_RNDU);
				mpfr_cosh(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			}
		} else {
			mpfr_cosh(fu_value, fu_value, MPFR_RNDU);
			if(mpfr_sgn(fl_value) == 0) mpfr_set_ui(fl_value, 1, MPFR_RNDD);
			else mpfr_cosh(fl_value, fl_value, MPFR_RNDD);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::acosh() {
	if(isPlusInfinity()) return true;
	if(isMinusInfinity()) return false;
	if(n_type == NUMBER_TYPE_FLOAT && (mpfr_inf_p(fl_value) && mpfr_sgn(fl_value) < 0)) return false;
	if(isOne()) {
		clear(true);
		return true;
	}
	if(hasImaginaryPart() || !isGreaterThanOrEqualTo(nr_one)) {
		if(b_imag) return false;
		if(isInterval(false)) {
			Number nr1(lowerEndPoint(true));
			Number nr2(upperEndPoint(true));
			if(!nr1.acosh() || !nr2.acosh()) return false;
			Number nr;
			if(!nr.setInterval(nr1, nr2, true)) return false;
			if(isInterval(true) && imaginaryPartIsInterval()) {
				Number nr3(lowerEndPoint(false));
				Number nr4(upperEndPoint(false));
				nr3.setImaginaryPart(i_value->upperEndPoint());
				nr4.setImaginaryPart(i_value->lowerEndPoint());
				if(!nr3.acosh() || !nr4.acosh()) return false;
				if(!nr.setInterval(nr, nr3, true)) return false;
				if(!nr.setInterval(nr, nr4, true)) return false;
			}
			if(hasRealPart() && !realPartIsNonZero()) {
				nr1 = lowerEndPoint(true);
				nr2 = upperEndPoint(true);
				nr1.clearReal();
				nr2.clearReal();
				if(!nr1.acosh() || !nr2.acosh()) return false;
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			if(hasImaginaryPart() && !imaginaryPartIsNonZero()) {
				nr1 = lowerEndPoint(false);
				nr2 = upperEndPoint(false);
				if(!nr1.acosh() || !nr2.acosh()) return false;
				Number nr_pi;
				nr_pi.pi();
				nr1.setImaginaryPart(nr_pi);
				nr_pi.negate();
				nr2.setImaginaryPart(nr_pi);
				if(!nr.setInterval(nr, nr1, true)) return false;
				if(!nr.setInterval(nr, nr2, true)) return false;
			}
			set(nr, true);
			return true;
		}
		if(CREATE_INTERVAL && !hasImaginaryPart()) {
			Number ipz(lowerEndPoint()), imz(ipz);
			if(!ipz.add(1) || !imz.subtract(1)) return false;
			if(!ipz.raise(nr_half) || !imz.raise(nr_half) || !ipz.multiply(imz) || !ipz.add(lowerEndPoint())) return false;
			Number ipz2(upperEndPoint()), imz2(ipz2);
			if(!ipz2.add(1) || !imz2.subtract(1)) return false;
			if(!ipz2.raise(nr_half) || !imz2.raise(nr_half) || !ipz2.multiply(imz2) || !ipz2.add(upperEndPoint())) return false;
			Number nriv;
			nriv.setInterval(ipz, ipz2);
			if(!nriv.ln()) return false;
			if(isGreaterThanOrEqualTo(nr_minus_one)) {
				nriv.clearReal();
			} else {
				if(nriv.isInterval(false) && nriv.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
			}
			set(nriv);
			return true;
		}
		Number ipz(*this), imz(*this);
		if(!ipz.add(1) || !imz.subtract(1)) return false;
		if(!ipz.raise(nr_half) || !imz.raise(nr_half) || !ipz.multiply(imz) || !ipz.add(*this) || !ipz.ln()) return false;
		if(hasImaginaryPart() && ipz.isInterval(false) && ipz.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
		set(ipz);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_acosh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_acosh(fl_value, fl_value, MPFR_RNDD);
		mpfr_acosh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::tan() {
	if(includesInfinity()) return false;
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*i_value, false, true);
			if(!t1a.multiply(2) || !t1b.multiply(2)) return false;
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.sin() || !t1b.sinh() || !t2a.cos() || !t2b.cosh() || !t2a.add(t2b) || !t1a.divide(t2a) ||  !t1b.divide(t2a)) return false;
			if(!t1a.isReal() || !t1b.isReal()) return false;
			if(t1a.isInterval(false) && t1a.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
			set(t1a, true, true);
			i_value->set(t1b, true, true);
			setPrecisionAndApproximateFrom(*i_value);
			testComplex(this, i_value);
			return true;
		} else {
			if(!i_value->tanh()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	bool do_pi = true;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(mpz_cmp_ui(mpq_denref(r_value), 1000000L) < 0) do_pi = false;
		if(!setToFloatingPoint()) return false;
	}
	if(mpfr_get_exp(fl_value) > BIT_PRECISION || mpfr_get_exp(fu_value) > BIT_PRECISION) {
		set(nr_bak);
		return false;
	}
	
	mpfr_clear_flags();
	
	if(!CREATE_INTERVAL && !isInterval()) {
		if(do_pi) {
			mpfr_t f_pi, f_quo;
			mpfr_init2(f_pi, BIT_PRECISION);
			mpfr_init2(f_quo, BIT_PRECISION - 30);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_quo, fl_value, f_pi, MPFR_RNDN);
			mpfr_frac(f_quo, f_quo, MPFR_RNDN);
			if(mpfr_zero_p(f_quo)) {
				clear(true);
				b_approx = true;
				if(i_precision < 0 || FROM_BIT_PRECISION(BIT_PRECISION - 30) < i_precision) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
				mpfr_clears(f_pi, f_quo, NULL);
				return true;
			}
			mpfr_clears(f_pi, f_quo, NULL);
		}
		mpfr_tan(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_t fl_pi, fu_pi, fu_quo, fl_quo, f_diff;
		mpfr_inits2(BIT_PRECISION, fl_pi, fu_pi, f_diff, fl_quo, fu_quo, NULL);
		mpfr_const_pi(fl_pi, MPFR_RNDD);
		mpfr_const_pi(fu_pi, MPFR_RNDU);
		bool b_neg1 = mpfr_sgn(fl_value) < 0;
		bool b_neg2 = mpfr_sgn(fu_value) < 0;
		mpfr_div(fl_quo, fl_value, b_neg1 ? fl_pi : fu_pi, MPFR_RNDD);
		mpfr_div(fu_quo, fu_value, b_neg2 ? fu_pi : fl_pi, MPFR_RNDU);
		mpfr_sub(f_diff, fu_quo, fl_quo, MPFR_RNDU);
		if(mpfr_cmp_ui(f_diff, 1) >= 0) {
			mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
			set(nr_bak);
			return false;
		}
		mpfr_frac(fl_quo, fl_quo, MPFR_RNDD);
		mpfr_frac(fu_quo, fu_quo, MPFR_RNDU);
		if(b_neg1) mpfr_neg(fl_quo, fl_quo, MPFR_RNDU);
		if(b_neg2) mpfr_neg(fu_quo, fu_quo, MPFR_RNDD);
		int c1 = mpfr_cmp_ui_2exp(fl_quo, 1, -1);
		int c2 = mpfr_cmp_ui_2exp(fu_quo, 1, -1);
		if(b_neg1) c1 = -c1;
		if(b_neg2) c2 = -c2;
		if((c1 != c2 && c1 <= 0 && c2 >= 0) || (c1 == c2 && mpfr_cmp_ui_2exp(f_diff, 1, -1) >= 0)) {
			mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
			set(nr_bak);
			return false;
		}
		mpfr_clears(f_diff, fl_pi, fu_pi, fl_quo, fu_quo, NULL);
		mpfr_tan(fl_value, fl_value, MPFR_RNDD);
		mpfr_tan(fu_value, fu_value, MPFR_RNDU);
	}

	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atan() {
	if(isZero()) return true;
	if(isInfinite(false)) {
		bool b_neg = isMinusInfinity();
		pi();
		divide(2);
		if(b_neg) negate();
		return true;
	}
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number ipz(*this), imz(*this);
			if(!ipz.multiply(nr_one_i) || !imz.multiply(nr_minus_i) || !ipz.add(1) || !imz.add(1)) return false;
			if(!ipz.ln() || !imz.ln() || !imz.subtract(ipz) || !imz.multiply(nr_one_i) || !imz.divide(2)) return false;
			if(imz.isInterval(false) && imz.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
			set(imz);
			return true;
		} else {
			Number nri(*i_value);
			if(!nri.atanh() || !nri.multiply(nr_one_i)) return false;
			set(nri, true);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_atan(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_atan(fl_value, fl_value, MPFR_RNDD);
		mpfr_atan(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atan2(const Number &o, bool allow_zero) {
	if(hasImaginaryPart() || o.hasImaginaryPart()) return false;
	if(isZero()) {
		if(allow_zero && o.isNonNegative()) {
			clear();
			setPrecisionAndApproximateFrom(o);
			return true;
		}
		if(o.isZero()) return false;
		if(o.isPositive()) {
			clear();
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(o.isFloatingPoint()) {
		if(!CREATE_INTERVAL && !isInterval() && !o.isInterval()) {
			mpfr_atan2(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			int sgn_l = mpfr_sgn(fl_value);
			int sgn_u = mpfr_sgn(fu_value);
			if(!allow_zero && !o.isNonZero() && (sgn_l != sgn_u || sgn_l == 0)) {set(nr_bak); return false;}
			int sgn_lo = mpfr_sgn(o.internalLowerFloat());
			int sgn_uo = mpfr_sgn(o.internalUpperFloat());
			if(sgn_lo < 0) {
				if(sgn_u >= 0) {
					if(sgn_l < 0) {
						mpfr_const_pi(fu_value, MPFR_RNDD);
						mpfr_neg(fu_value, fu_value, MPFR_RNDD);
					} else if(sgn_uo >= 0) {
						if(sgn_l == 0) mpfr_set_zero(fu_value, 0);
						else mpfr_atan2(fu_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
					} else {
						mpfr_atan2(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDD);
					}
					if(sgn_l <= 0) mpfr_const_pi(fl_value, MPFR_RNDU);
					else mpfr_atan2(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDU);
					mpfr_swap(fl_value, fu_value);
				} else {
					mpfr_atan2(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDU);
					mpfr_atan2(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDD);
					mpfr_swap(fl_value, fu_value);
				}
			} else {
				if(sgn_u >= 0) {
					if(sgn_u == 0) mpfr_set_zero(fu_value, 0);
					else mpfr_atan2(fu_value, fu_value, o.internalLowerFloat(), MPFR_RNDU);
					if(sgn_l == 0) mpfr_set_zero(fl_value, 0);
					else if(sgn_l < 0) mpfr_atan2(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
					else mpfr_atan2(fl_value, fl_value, o.internalUpperFloat(), MPFR_RNDD);
				} else {
					mpfr_atan2(fl_value, fl_value, o.internalLowerFloat(), MPFR_RNDD);
					mpfr_atan2(fu_value, fu_value, o.internalUpperFloat(), MPFR_RNDU);
				}
			}
		}
	} else {
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_t of_value;
			mpfr_init2(of_value, BIT_PRECISION);
			if(o.isPlusInfinity()) mpfr_set_inf(of_value, 1);
			else if(o.isMinusInfinity()) mpfr_set_inf(of_value, -1);
			else mpfr_set_q(of_value, o.internalRational(), MPFR_RNDN);
			mpfr_atan2(fl_value, fl_value, of_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
			mpfr_clear(of_value);
		} else {
			Number nr_o(o);
			if(!nr_o.setToFloatingPoint() || !atan2(nr_o)) return false;
			return true;
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::arg() {
	if(!isNonZero()) return false;
	if(!hasImaginaryPart()) {
		if(isNegative()) {
			pi();
		} else {
			clear(true);
		}
		return true;
	}
	if(!hasRealPart()) {
		bool b_neg = i_value->isNegative();
		pi();
		multiply(nr_half);
		if(b_neg) negate();
		return true;
	}
	Number *i_value2 = i_value;
	i_value = NULL;
	if(!i_value2->atan2(*this)) {
		i_value = i_value2;
		return false;
	}
	set(*i_value2);
	delete i_value2;
	return true;
}
bool Number::tanh() {
	if(isPlusInfinity()) {set(1, 1, 0, true); return true;}
	if(isMinusInfinity()) {set(-1, 1, 0, true); return true;}
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) {
			Number t1a, t1b, t2a, t2b;
			t1a.set(*this, false, true);
			t1b.set(*this, false);
			t1b.clearReal();
			if(!t1a.tanh() || !t1b.tanh()) return false;
			t2a.set(t1a);
			t2b.set(t1b);
			if(!t1a.add(t1b) || !t2a.multiply(t2b) || !t2a.add(1) || !t1a.divide(t2a)) return false;
			if(t1a.isInterval(false) && t1a.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
			set(t1a, true);
			return true;
		} else {
			if(!i_value->tan()) return false;
			setPrecisionAndApproximateFrom(*i_value);
			return true;
		}
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_tanh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_tanh(fl_value, fl_value, MPFR_RNDD);
		mpfr_tanh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::atanh() {
	if(isZero()) return true;
	if(isOne()) {
		if(b_imag) return false;
		setPlusInfinity();
		return true;
	}
	if(isMinusOne()) {
		if(b_imag) return false;
		setMinusInfinity();
		return true;
	}
	if(hasImaginaryPart() || !isLessThanOrEqualTo(1) || !isGreaterThanOrEqualTo(-1)) {
		if(b_imag) return false;
		if(!hasImaginaryPart()) {
			Number nr_bak(*this);
			if(!setToFloatingPoint()) return false;
			mpfr_clear_flags();
			Number i_nr;
			i_nr.markAsImaginaryPart();
			int cmp1u = mpfr_cmp_si(fu_value, 1);
			int cmp1l = mpfr_cmp_si(fl_value, 1);
			int cmp0u = mpfr_sgn(fu_value);
			int cmp0l = mpfr_sgn(fl_value);
			int cmpm1u = mpfr_cmp_si(fu_value, -1);
			int cmpm1l = mpfr_cmp_si(fl_value, -1);
			if(cmp1u > 0) {
				i_nr.pi();
				if(!i_nr.multiply(nr_minus_half)) {set(nr_bak); return false;}
				if(cmpm1l < 0) {
					Number nr;
					nr.pi();
					if(!nr.multiply(nr_half) || !i_nr.setInterval(i_nr, nr)) {set(nr_bak); return false;}
				} else if(cmp1l <= 0) {
					if(!i_nr.setInterval(i_nr, nr_zero)) {set(nr_bak); return false;}
				}
			} else {
				i_nr.pi();
				if(!i_nr.multiply(nr_half)) {set(nr_bak); return false;}
				if(cmp1u > 0) {
					Number nr;
					nr.pi();
					if(!nr.multiply(nr_minus_half) || !i_nr.setInterval(nr, i_nr)) {set(nr_bak); return false;}
				} else if(cmpm1u >= 0) {
					if(!i_nr.setInterval(nr_zero, i_nr)) {set(nr_bak); return false;}
				}
			}
			if((cmp1u >= 0 && cmp1l >= 0) || (cmpm1u <= 0 && cmpm1l <= 0)) {
				if(!recip() || !atanh()) {set(nr_bak); return false;}
			} else if(cmp1u >= 0 && cmpm1l <= 0) {
				mpfr_set_inf(fl_value, -1);
				mpfr_set_inf(fu_value, 1);
			} else if(cmp1u <= 0) {
				if(cmp0u < 0) {
					mpfr_ui_div(fl_value, 1, fl_value, MPFR_RNDU);
					if(mpfr_cmp(fl_value, fu_value) > 0) {
						mpfr_swap(fl_value, fu_value);
					}
				}
				mpfr_atanh(fu_value, fu_value, MPFR_RNDU);
				mpfr_set_inf(fl_value, -1);
			} else {
				if(cmp0l > 0) {
					mpfr_ui_div(fu_value, 1, fu_value, MPFR_RNDD);
					if(mpfr_cmp(fu_value, fl_value) < 0) {
						mpfr_swap(fl_value, fu_value);
					}
				}
				mpfr_atanh(fl_value, fl_value, MPFR_RNDD);
				mpfr_set_inf(fu_value, 1);
			}
			if(!i_value) {i_value = new Number(i_nr); i_value->markAsImaginaryPart();}
			else i_value->set(i_nr, true);
			setPrecisionAndApproximateFrom(*i_value);
			if(!testFloatResult()) {
				set(nr_bak);
				return false;
			}
			return true;
		} else if(!hasRealPart()) {
			Number inr(*i_value);
			if(!inr.atan() || !inr.multiply(nr_one_i)) return false;
			set(inr, true);
			return true;
		}
		Number ipz(nr_one), imz(nr_one);
		if(!ipz.add(*this) || !imz.subtract(*this) || !ipz.ln() || !imz.ln() || !imz.negate() || !ipz.add(imz) || !ipz.divide(2)) return false;
		if(ipz.isInterval(false) && ipz.precision(1) <= PRECISION + 20) CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval calculated wide."), NULL);
		set(ipz);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		if(mpfr_cmp_si(fl_value, -1) == 0) mpfr_set_inf(fl_value, -1);
		else if(mpfr_cmp_si(fl_value, 1) == 0) mpfr_set_inf(fl_value, 1);
		else mpfr_atanh(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_cmp_si(fl_value, -1) == 0) mpfr_set_inf(fl_value, -1);
		else if(mpfr_cmp_si(fl_value, 1) == 0) mpfr_set_inf(fl_value, 1);
		else mpfr_atanh(fl_value, fl_value, MPFR_RNDD);
		if(mpfr_cmp_si(fu_value, -1) == 0) mpfr_set_inf(fu_value, -1);
		else if(mpfr_cmp_si(fu_value, 1) == 0) mpfr_set_inf(fu_value, 1);
		else mpfr_atanh(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::ln() {

	if(isPlusInfinity()) return true;
	if(isMinusInfinity()) {
		n_type = NUMBER_TYPE_PLUS_INFINITY;
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->pi();
		return true;
	}

	if(isOne()) {
		clear(true);
		return true;
	}

	if(isZero()) {
		if(b_imag) return false;
		setMinusInfinity();
		return true;
	}


	if(hasImaginaryPart()) {
		Number new_i(*i_value);
		Number new_r(*this);
		Number this_r;
		this_r.set(*this, false, true);
		if(!new_i.atan2(this_r, true) || !new_r.abs() || new_i.hasImaginaryPart()) return false;
		if(new_r.hasImaginaryPart() || !new_r.ln()) return false;
		set(new_r);
		setImaginaryPart(new_i);
		testComplex(this, i_value);
		return true;
	} else if(isNonPositive()) {
		if(b_imag) return false;
		Number new_r(*this);
		if(!new_r.abs() || !new_r.ln()) return false;
		set(new_r);
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->pi();
		testComplex(this, i_value);
		return true;
	}

	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
		
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_log(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) < 0) {
			if(mpfr_cmpabs(fl_value, fu_value) > 0) {
				mpfr_neg(fu_value, fl_value, MPFR_RNDU);
			}
			mpfr_set_inf(fl_value, -1);
			mpfr_log(fu_value, fu_value, MPFR_RNDU);
			if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
			i_value->pi();
			i_value->setInterval(nr_zero, *i_value);
		} else {
			if(mpfr_zero_p(fl_value)) mpfr_set_inf(fl_value, -1);
			else mpfr_log(fl_value, fl_value, MPFR_RNDD);
			mpfr_log(fu_value, fu_value, MPFR_RNDU);
		}
	}

	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}

	return true;
}
bool Number::log(const Number &o) {
	if(isOne() && !o.isGreaterThan(1)) {
		clear(true);
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isOne()) return false;
	if(o.isRational() && o == 2 && isReal() && isPositive()) {
		Number nr_bak(*this);
		if(!setToFloatingPoint()) return false;
		mpfr_clear_flags();
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_log2(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_log2(fl_value, fl_value, MPFR_RNDD);
			mpfr_log2(fu_value, fu_value, MPFR_RNDU);
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	} else if(o.isRational() && o == 10 && isReal() && isPositive()) {
		Number nr_bak(*this);
		if(!setToFloatingPoint()) return false;
		mpfr_clear_flags();
		if(!CREATE_INTERVAL && !isInterval()) {
			mpfr_log10(fl_value, fl_value, MPFR_RNDN);
			mpfr_set(fu_value, fl_value, MPFR_RNDN);
		} else {
			mpfr_log10(fl_value, fl_value, MPFR_RNDD);
			mpfr_log10(fu_value, fu_value, MPFR_RNDU);
		}
		if(!testFloatResult(true)) {
			set(nr_bak);
			return false;
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	Number num(*this);
	Number den(o);
	if(!num.ln() || !den.ln() || !den.recip() || !num.multiply(den)) return false;
	if(b_imag && num.hasImaginaryPart()) return false;
	set(num);
	return true;
}
bool Number::exp() {
	if(isPlusInfinity()) return true;
	if(isMinusInfinity()) {
		clear();
		return true;
	}
	if(hasImaginaryPart()) {
		Number e_base;
		e_base.e();
		if(!e_base.raise(*this)) return false;
		set(e_base);
		return true;
	}
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_exp(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_exp(fu_value, fu_value, MPFR_RNDU);
		mpfr_exp(fl_value, fl_value, MPFR_RNDD);
	}
	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::lambertW() {

	if(!isReal()) return false;
	if(isZero()) return true;
	
	if(isInterval()) {
		Number nr_l, nr_u;
		nr_l.setInternal(fl_value);
		nr_u.setInternal(fu_value);
		nr_l.intervalToPrecision();
		nr_u.intervalToPrecision();
		if(!nr_l.lambertW() || !nr_u.lambertW()) return false;
		setPrecisionAndApproximateFrom(nr_l);
		setPrecisionAndApproximateFrom(nr_u);
		return setInterval(nr_l, nr_u, true);
	}
	
	Number nr_bak(*this);
	mpfr_clear_flags();
	
	mpfr_t x, m1_div_exp1;
	mpfr_inits2(BIT_PRECISION, x, m1_div_exp1, NULL);
	if(n_type == NUMBER_TYPE_RATIONAL) mpfr_set_q(x, r_value, MPFR_RNDN);
	else mpfr_set(x, fl_value, MPFR_RNDN);
	mpfr_set_ui(m1_div_exp1, 1, MPFR_RNDN);
	mpfr_exp(m1_div_exp1, m1_div_exp1, MPFR_RNDN);
	mpfr_ui_div(m1_div_exp1, 1, m1_div_exp1, MPFR_RNDN);
	mpfr_neg(m1_div_exp1, m1_div_exp1, MPFR_RNDN);
	int cmp = mpfr_cmp(x, m1_div_exp1);
	if(cmp == 0) {
		mpfr_clears(x, m1_div_exp1, NULL);
		if(!CREATE_INTERVAL) {
			set(-1, 1);
			b_approx = true;
			i_precision = PRECISION;
			return true;
		} else {
			mpfr_set_ui(fl_value, -1, MPFR_RNDD);
			mpfr_set_ui(fu_value, -1, MPFR_RNDU);
		}
	} else if(cmp < 0) {
		mpfr_clears(x, m1_div_exp1, NULL);
		set(nr_bak);
		return false;
	} else {
		mpfr_t w;
		mpfr_init2(w, BIT_PRECISION);
		mpfr_set_zero(w, 0);
		cmp = mpfr_cmp_ui(x, 10);
		if(cmp > 0) {
			mpfr_log(w, x, MPFR_RNDN);
			mpfr_t wln;
			mpfr_init2(wln, BIT_PRECISION);
			mpfr_log(wln, w, MPFR_RNDN);
			mpfr_sub(w, w, wln, MPFR_RNDN);
			mpfr_clear(wln);
		}
		
		mpfr_t wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2;
		mpfr_inits2(BIT_PRECISION, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
		mpfr_set_si(wPrec, -(BIT_PRECISION - 30), MPFR_RNDN);
		mpfr_exp2(wPrec, wPrec, MPFR_RNDN);
		while(true) {
			if(CALCULATOR->aborted() || testErrors()) {
				mpfr_clears(x, m1_div_exp1, w, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
				set(nr_bak);
				return false;
			}
			mpfr_exp(wTimesExpW, w, MPFR_RNDN);
			mpfr_set(wPlusOneTimesExpW, wTimesExpW, MPFR_RNDN);
			mpfr_mul(wTimesExpW, wTimesExpW, w, MPFR_RNDN);
			mpfr_add(wPlusOneTimesExpW, wPlusOneTimesExpW, wTimesExpW, MPFR_RNDN);
			mpfr_sub(testXW, x, wTimesExpW, MPFR_RNDN);
			mpfr_div(testXW, testXW, wPlusOneTimesExpW, MPFR_RNDN);
			mpfr_abs(testXW, testXW, MPFR_RNDN);
			if(mpfr_cmp(wPrec, testXW) > 0) {
				break;
			}
			mpfr_sub(wTimesExpW, wTimesExpW, x, MPFR_RNDN);
			mpfr_add_ui(tmp1, w, 2, MPFR_RNDN);
			mpfr_mul(tmp2, wTimesExpW, tmp1, MPFR_RNDN);
			mpfr_mul_ui(tmp1, w, 2, MPFR_RNDN);
			mpfr_add_ui(tmp1, tmp1, 2, MPFR_RNDN);
			mpfr_div(tmp2, tmp2, tmp1, MPFR_RNDN);
			mpfr_sub(wPlusOneTimesExpW, wPlusOneTimesExpW, tmp2, MPFR_RNDN);
			mpfr_div(wTimesExpW, wTimesExpW, wPlusOneTimesExpW, MPFR_RNDN);
			mpfr_sub(w, w, wTimesExpW, MPFR_RNDN);
		}
		if(n_type == NUMBER_TYPE_RATIONAL) {
			mpfr_init2(fl_value, BIT_PRECISION);
			mpfr_init2(fu_value, BIT_PRECISION);
			n_type = NUMBER_TYPE_FLOAT;
			mpq_set_ui(r_value, 0, 1);
		}
		mpfr_set(fl_value, w, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
		mpfr_clears(x, m1_div_exp1, w, wPrec, wTimesExpW, wPlusOneTimesExpW, testXW, tmp1, tmp2, NULL);
		if(i_precision < 0 || i_precision > PRECISION) i_precision = FROM_BIT_PRECISION(BIT_PRECISION - 30);
		if(CREATE_INTERVAL) {
			precisionToInterval();
		}
	}
	if(!testFloatResult(true)) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;

}
bool Number::gcd(const Number &o) {
	if(!isInteger() || !o.isInteger()) {
		return false;
	}
	if(isZero() && o.isZero()) {
		clear(); 
		return true;
	}
	mpz_gcd(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::lcm(const Number &o) {
	if(isInteger() && o.isInteger()) {
		mpz_lcm(mpq_numref(r_value), mpq_numref(r_value), mpq_numref(o.internalRational()));
		return true;
	}
	return multiply(o);
}

bool Number::polylog(const Number &o) {
	if(isZero()) return true;
	if(!o.isTwo() || !isLessThanOrEqualTo(1)) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_li2(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_li2(fl_value, fl_value, MPFR_RNDD);
		mpfr_li2(fu_value, fu_value, MPFR_RNDU);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::igamma(const Number &o) {
#if MPFR_VERSION_MAJOR < 4
	return false;
#else
	if(!isReal() || (!o.isNonZero() && !isNonZero())) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	Number o_float(o);
	if(!o_float.setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval() && !o_float.isInterval()) {
		mpfr_gamma_inc(fl_value, fl_value, o_float.internalLowerFloat(), MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		mpfr_gamma_inc(fl_value, fl_value, o_float.internalUpperFloat(), MPFR_RNDD);
		mpfr_gamma_inc(fu_value, fu_value, o_float.internalLowerFloat(), MPFR_RNDU);
		if(!o.isGreaterThanOrEqualTo(1) && !nr_bak.isGreaterThan(2) && nr_bak.isInterval() && nr_bak.precision(1) <= PRECISION + 20) CALCULATOR->error(false, _("%s() lacks proper support interval arithmetic."), CALCULATOR->f_igamma->name().c_str(), NULL);
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
#endif
}
bool Number::expint() {
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(!setToFloatingPoint()) return false;
	mpfr_clear_flags();
	if(!CREATE_INTERVAL && !isInterval()) {
		mpfr_eint(fl_value, fl_value, MPFR_RNDN);
		mpfr_set(fu_value, fl_value, MPFR_RNDN);
	} else {
		if(mpfr_sgn(fl_value) < 0) {
			if(mpfr_sgn(fu_value) > 0) {
				mpfr_eint(fl_value, fl_value, MPFR_RNDU);
				mpfr_eint(fu_value, fu_value, MPFR_RNDU);
				if(mpfr_cmp(fl_value, fu_value) > 0) mpfr_swap(fl_value, fu_value);
				mpfr_set_inf(fl_value, -1);
			} else {
				mpfr_eint(fl_value, fl_value, MPFR_RNDU);
				mpfr_eint(fu_value, fu_value, MPFR_RNDD);
				mpfr_swap(fl_value, fu_value);
			}
		} else {
			mpfr_eint(fl_value, fl_value, MPFR_RNDD);
			mpfr_eint(fu_value, fu_value, MPFR_RNDU);
		}
	}
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::logint() {
	if(isZero()) return true;
	Number nr_bak(*this);
	if(!ln() || !expint()) {
		set(nr_bak);
		return false;
	}
	return true;
}
bool Number::sinint() {
	if(isPlusInfinity()) {pi(); multiply(2); return true;}
	if(isMinusInfinity()) {pi(); multiply(-2); return true;}
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) return false;
		if(!i_value->sinhint()) return false;
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	Number nr_bak(*this);
	if(isNegative()) {
		if(!negate() || !sinint() || !negate()) {set(nr_bak); return false;}
		return true;
	}
	if(!setToFloatingPoint()) return false;
	Number nr_round(*this);
	nr_round.round();
	if(isInterval()) {
		Number nr_lower(lowerEndPoint());
		Number nr_upper(upperEndPoint());
		if(nr_lower.isNegative() && nr_upper.isPositive()) {
			nr_lower.setInterval(nr_lower, nr_zero);
			nr_upper.setInterval(nr_zero, nr_upper);
			if(!nr_lower.sinint() || !nr_upper.sinint()) {
				set(nr_bak);
				return false;
			}
			setInterval(nr_lower, nr_upper);
			return true;
		}
		Number nr_mid1;
		Number nr_mid2;
		bool b1 = false, b2 = false, b = true;
		if(mpfr_cmp_si(fl_value, 1) > 0) {
			mpfr_t f_pi, f_mid1, f_mid2;
			if(mpfr_get_exp(fl_value) > 1000 * PRECISION) {
				pi();
				divide(2);
				return true;
			}
			if(mpfr_get_exp(fl_value) > 100000) {
				set(nr_bak);
				return false;
			}
			mpfr_prec_t prec = mpfr_get_prec(fl_value) + mpfr_get_exp(fl_value);
			if(nr_round > 100 + PRECISION * 5) prec += nr_round.intValue() * 5;
			else prec += 10;
			mpfr_inits2(mpfr_get_prec(fl_value) + mpfr_get_exp(fl_value), f_pi, f_mid1, f_mid2, NULL);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_mid1, nr_lower.internalLowerFloat(), f_pi, MPFR_RNDN);
			mpfr_ceil(f_mid1, f_mid1);
			mpfr_add_ui(f_mid2, f_mid1, 1, MPFR_RNDN);
			mpfr_mul(f_mid1, f_mid1, f_pi, MPFR_RNDN);
			if(mpfr_cmp(f_mid1, nr_upper.internalLowerFloat()) < 0) {
				mpfr_mul(f_mid2, f_mid2, f_pi, MPFR_RNDN);
				b1 = true;
				b2 = mpfr_cmp(f_mid2, nr_upper.internalLowerFloat()) < 0;
			}
			mpfr_clear_flags();
			for(size_t i2 = 0; b && ((i2 == 0 && b1) || (i2 == 1 && b2)); i2++) {
				b = false;
				mpfr_t f_x, f_xi, f_y;
				mpz_t z_i, z_fac;
				mpz_inits(z_i, z_fac, NULL);
				if(nr_round > 100 + PRECISION * 5) {
					mpfr_t f_sin, f_cos, f_y1, f_y2, f_yt, f_pi;
					mpfr_inits2(mpfr_get_prec(i2 == 0 ? f_mid1 : f_mid2), f_x, f_xi, f_y, NULL);
					mpfr_set(f_x, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpfr_inits2(mpfr_get_prec(f_x), f_pi, f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
					mpfr_const_pi(f_pi, MPFR_RNDN);
					mpfr_div_ui(f_pi, f_pi, 2, MPFR_RNDN);
					mpfr_set(f_cos, f_x, MPFR_RNDN);
					mpfr_set(f_sin, f_x, MPFR_RNDN);
					mpfr_cos(f_cos, f_cos, MPFR_RNDN);
					mpfr_div(f_cos, f_cos, f_x, MPFR_RNDN);
					mpfr_sin(f_sin, f_sin, MPFR_RNDN);
					mpfr_div(f_sin, f_sin, f_x, MPFR_RNDN);
					mpfr_set_ui(f_y1, 1, MPFR_RNDN);
					mpfr_ui_div(f_y2, 1, f_x, MPFR_RNDN);
					mpz_set_ui(z_i, 1);
					mpz_set_ui(z_fac, 1);
					for(size_t i = 0; i < 100; i++) {
						if(CALCULATOR->aborted()) {set(nr_bak); return false;}
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
						mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						if(i % 2 == 0) mpfr_sub(f_y1, f_y1, f_xi, MPFR_RNDN);
						else mpfr_add(f_y1, f_y1, f_xi, MPFR_RNDN);
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
						mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						if(i % 2 == 0) mpfr_sub(f_y2, f_y2, f_xi, MPFR_RNDN);
						else mpfr_add(f_y2, f_y2, f_xi, MPFR_RNDN);
						mpfr_mul(f_yt, f_y1, f_cos, MPFR_RNDN);
						mpfr_sub(f_y, f_pi, f_yt, MPFR_RNDN);
						mpfr_mul(f_yt, f_y2, f_sin, MPFR_RNDN);
						mpfr_sub(f_y, f_y, f_yt, MPFR_RNDN);
						mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
						if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
						mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
					}
					mpfr_clears(f_sin, f_cos, f_y1, f_y2, f_yt, f_pi, NULL);
				} else {
					mpfr_inits2(mpfr_get_prec(i2 == 0 ? f_mid1 : f_mid2), f_x, f_xi, f_y, NULL);
					mpfr_set(f_x, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpfr_set(f_y, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpz_set_ui(z_i, 1);
					mpz_set_ui(z_fac, 1);
					for(size_t i = 0; i < 10000; i++) {
						if(CALCULATOR->aborted()) {set(nr_bak); return false;}
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						mpz_divexact(z_fac, z_fac, z_i);
						if(i % 2 == 0) mpfr_sub(f_y, f_y, f_xi, MPFR_RNDN);
						else mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
						mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
						if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
						mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
					}
				}
				if(b) {
					if(i2 == 0) {
						nr_mid1.setInternal(fl_value);
						mpfr_set(nr_mid1.internalUpperFloat(), f_y, MPFR_RNDU);
						if(!nr_mid1.testFloatResult()) b = false;
					} else {
						nr_mid2.setInternal(fl_value);
						mpfr_set(nr_mid2.internalUpperFloat(), f_y, MPFR_RNDU);
						if(!nr_mid2.testFloatResult()) b = false;
					}
				}
				mpfr_clears(f_x, f_xi, f_y, NULL);
				mpz_clears(z_i, z_fac, NULL);
			}
			mpfr_clears(f_pi, f_mid1, f_mid2, NULL);
		}
		if(!b || !nr_lower.sinint() || !nr_upper.sinint()) {
			set(nr_bak);
			return false;
		}
		setInterval(nr_lower, nr_upper);
		if(b1) setInterval(*this, nr_mid1);
		if(b2) setInterval(*this, nr_mid2);
		return true;
	}
	mpfr_clear_flags();
	mpfr_t f_x, f_xi, f_y;
	mpz_t z_i, z_fac;
	mpz_inits(z_i, z_fac, NULL);
	bool b = false;
	if(nr_round > 100 + PRECISION * 5) {
		mpfr_t f_sin, f_cos, f_y1, f_y2, f_yt, f_pi;
		mpfr_inits2(mpfr_get_prec(fl_value) + 10, f_x, f_xi, f_y, NULL);
		mpfr_set(f_x, fl_value, MPFR_RNDN);
		mpfr_inits2(mpfr_get_prec(f_x), f_pi, f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
		mpfr_const_pi(f_pi, MPFR_RNDN);
		mpfr_div_ui(f_pi, f_pi, 2, MPFR_RNDN);
		mpfr_set(f_cos, f_x, MPFR_RNDN);
		mpfr_set(f_sin, f_x, MPFR_RNDN);
		mpfr_cos(f_cos, f_cos, MPFR_RNDN);
		mpfr_div(f_cos, f_cos, f_x, MPFR_RNDN);
		mpfr_sin(f_sin, f_sin, MPFR_RNDN);
		mpfr_div(f_sin, f_sin, f_x, MPFR_RNDN);
		mpfr_set_ui(f_y1, 1, MPFR_RNDN);
		mpfr_ui_div(f_y2, 1, f_x, MPFR_RNDN);
		mpz_set_ui(z_i, 1);
		mpz_set_ui(z_fac, 1);
		for(size_t i = 0; i < 100; i++) {
			if(CALCULATOR->aborted()) {set(nr_bak); return false;}
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
			mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			if(i % 2 == 0) mpfr_sub(f_y1, f_y1, f_xi, MPFR_RNDN);
			else mpfr_add(f_y1, f_y1, f_xi, MPFR_RNDN);
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
			mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			if(i % 2 == 0) mpfr_sub(f_y2, f_y2, f_xi, MPFR_RNDN);
			else mpfr_add(f_y2, f_y2, f_xi, MPFR_RNDN);
			mpfr_mul(f_yt, f_y1, f_cos, MPFR_RNDN);
			mpfr_sub(f_y, f_pi, f_yt, MPFR_RNDN);
			mpfr_mul(f_yt, f_y2, f_sin, MPFR_RNDN);
			mpfr_sub(f_y, f_y, f_yt, MPFR_RNDN);
			mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
			if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
			mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		}
		mpfr_clears(f_sin, f_cos, f_y1, f_y2, f_yt, f_pi, NULL);
	} else {
		mpfr_inits2(mpfr_get_prec(fl_value) + nr_round.intValue() * 5, f_x, f_xi, f_y, NULL);
		mpfr_set(f_x, fl_value, MPFR_RNDN);
		mpfr_set(f_y, fl_value, MPFR_RNDN);
		mpz_set_ui(z_i, 1);
		mpz_set_ui(z_fac, 1);
		for(size_t i = 0; i < 10000; i++) {
			if(CALCULATOR->aborted()) {set(nr_bak); return false;}
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			mpz_divexact(z_fac, z_fac, z_i);
			if(i % 2 == 0) mpfr_sub(f_y, f_y, f_xi, MPFR_RNDN);
			else mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
			mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
			if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
			mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		}
	}
	if(!b) {
		set(nr_bak);
		return false;
	}
	if(CREATE_INTERVAL) mpfr_set(fu_value, f_y, MPFR_RNDU);
	else mpfr_set(fu_value, fl_value, MPFR_RNDN);
	mpfr_clears(f_x, f_xi, f_y, NULL);
	mpz_clears(z_i, z_fac, NULL);
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;
}
bool Number::sinhint() {
	if(isPlusInfinity()) {return true;}
	if(isMinusInfinity()) {return true;}
	if(isZero()) return true;
	if(hasImaginaryPart()) {
		if(hasRealPart()) return false;
		if(!i_value->sinhint()) return false;
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	Number nr_bak(*this);
	if(isNegative()) {
		if(!negate() || !sinhint() || !negate()) {set(nr_bak); return false;}
		return true;
	}
	if(isGreaterThan(1000)) return false;
	if(!setToFloatingPoint()) return false;
	if(isInterval()) {
		Number nr_lower(lowerEndPoint());
		Number nr_upper(upperEndPoint());
		if(!nr_lower.sinhint() || !nr_upper.sinhint()) {
			set(nr_bak);
			return false;
		}
		setInterval(nr_lower, nr_upper);
		return true;
	}
	mpfr_clear_flags();
	mpfr_t f_x, f_xi, f_y;
	mpz_t z_i, z_fac;
	mpz_inits(z_i, z_fac, NULL);
	Number nr_round(*this);
	nr_round.round();
	mpfr_inits2(mpfr_get_prec(fl_value) + nr_round.intValue(), f_x, f_xi, f_y, NULL);
	mpfr_set(f_x, fl_value, MPFR_RNDN);
	mpfr_set(f_y, fl_value, MPFR_RNDN);
	mpz_set_ui(z_i, 1);
	mpz_set_ui(z_fac, 1);
	bool b = false;
	for(size_t i = 0; i < 10000; i++) {
		if(CALCULATOR->aborted()) {set(nr_bak); return false;}
		mpz_add_ui(z_i, z_i, 1);
		mpz_mul(z_fac, z_fac, z_i);
		mpz_add_ui(z_i, z_i, 1);
		mpz_mul(z_fac, z_fac, z_i);
		mpz_mul(z_fac, z_fac, z_i);
		mpfr_set(f_xi, f_x, MPFR_RNDN);
		mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
		mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
		mpz_divexact(z_fac, z_fac, z_i);
		mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
		mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
		mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
	}
	if(!b) {
		set(nr_bak);
		return false;
	}
	if(CREATE_INTERVAL) mpfr_set(fu_value, f_y, MPFR_RNDU);
	else mpfr_set(fu_value, fl_value, MPFR_RNDN);
	mpfr_clears(f_x, f_xi, f_y, NULL);
	mpz_clears(z_i, z_fac, NULL);
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;
}
bool Number::cosint() {
	if(isPlusInfinity()) {clear(true); return true;}
	if(isZero()) {setMinusInfinity(true); return true;}
	if(hasImaginaryPart() && !hasRealPart()) {
		if(i_value->isNegative()) {
			set(*i_value, true);
			negate();
			if(!coshint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(-2);
			setImaginaryPart(nr_i);
			return true;
		} else if(i_value->isPositive()) {
			set(*i_value, true);
			if(!coshint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(2);
			setImaginaryPart(nr_i);
			return true;
		} else {
			set(*i_value, true);
			if(!coshint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(2);
			Number nr_low(nr_i);
			nr_low.negate();
			nr_i.setInterval(nr_low, nr_i);
			setImaginaryPart(nr_i);
			return true;
		}
	}
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(isNegative()) {
		if(!negate() || !cosint() || !negate()) {set(nr_bak); return false;}
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->pi();
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	if(!setToFloatingPoint()) return false;
	Number nr_round(*this);
	nr_round.round();
	if(isInterval()) {
		Number nr_lower(lowerEndPoint());
		Number nr_upper(upperEndPoint());
		if(nr_lower.isNegative() && nr_upper.isPositive()) {
			nr_lower.setInterval(nr_lower, nr_zero);
			nr_upper.setInterval(nr_zero, nr_upper);
			if(!nr_lower.cosint() || !nr_upper.cosint()) {
				set(nr_bak);
				return false;
			}
			setInterval(nr_lower, nr_upper);
			return true;
		}
		Number nr_mid1;
		Number nr_mid2;
		bool b1 = false, b2 = false, b = true;
		if(mpfr_cmp_si(fl_value, 1) > 0) {
			mpfr_t f_pi, f_mid1, f_mid2;
			if(mpfr_get_exp(fl_value) > 100000) {
				set(nr_bak);
				return false;
			}
			mpfr_prec_t prec = mpfr_get_prec(fl_value) + mpfr_get_exp(fl_value);
			if(nr_round > 100 + PRECISION * 5) prec += nr_round.intValue() * 5;
			else prec += 10;
			mpfr_inits2(mpfr_get_prec(fl_value) + mpfr_get_exp(fl_value), f_pi, f_mid1, f_mid2, NULL);
			mpfr_const_pi(f_pi, MPFR_RNDN);
			mpfr_div(f_mid1, nr_lower.internalLowerFloat(), f_pi, MPFR_RNDN);
			mpfr_floor(f_mid1, f_mid1);
			mpfr_mul_ui(f_mid1, f_mid1, 2, MPFR_RNDN);
			mpfr_add_ui(f_mid1, f_mid1, 1, MPFR_RNDN);
			mpfr_div_ui(f_mid1, f_mid1, 2, MPFR_RNDN);
			mpfr_add_ui(f_mid2, f_mid1, 1, MPFR_RNDN);
			mpfr_mul(f_mid1, f_mid1, f_pi, MPFR_RNDN);
			if(mpfr_cmp(f_mid1, nr_lower.internalLowerFloat()) < 0) {
				mpfr_add(f_mid1, f_mid1, f_pi, MPFR_RNDN);
				mpfr_add_ui(f_mid2, f_mid2, 1, MPFR_RNDN);
			}
			if(mpfr_cmp(f_mid1, nr_upper.internalLowerFloat()) < 0) {
				mpfr_mul(f_mid2, f_mid2, f_pi, MPFR_RNDN);
				b1 = true;
				b2 = mpfr_cmp(f_mid2, nr_upper.internalLowerFloat()) < 0;
			}
			mpfr_clear_flags();
			for(size_t i2 = 0; b && ((i2 == 0 && b1) || (i2 == 1 && b2)); i2++) {
				b = false;
				mpfr_t f_x, f_xi, f_y;
				mpz_t z_i, z_fac;
				mpz_inits(z_i, z_fac, NULL);
				if(nr_round > 100 + PRECISION * 5) {
					mpfr_t f_sin, f_cos, f_y1, f_y2, f_yt;
					mpfr_inits2(mpfr_get_prec(i2 == 0 ? f_mid1 : f_mid2), f_x, f_xi, f_y, NULL);
					mpfr_set(f_x, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpfr_inits2(mpfr_get_prec(f_x), f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
					mpfr_set(f_cos, f_x, MPFR_RNDN);
					mpfr_set(f_sin, f_x, MPFR_RNDN);
					mpfr_cos(f_cos, f_cos, MPFR_RNDN);
					mpfr_div(f_cos, f_cos, f_x, MPFR_RNDN);
					mpfr_sin(f_sin, f_sin, MPFR_RNDN);
					mpfr_div(f_sin, f_sin, f_x, MPFR_RNDN);
					mpfr_set_ui(f_y1, 1, MPFR_RNDN);
					mpfr_ui_div(f_y2, 1, f_x, MPFR_RNDN);
					mpz_set_ui(z_i, 1);
					mpz_set_ui(z_fac, 1);
					for(size_t i = 0; i < 100; i++) {
						if(CALCULATOR->aborted()) {set(nr_bak); return false;}
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
						mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						if(i % 2 == 0) mpfr_sub(f_y1, f_y1, f_xi, MPFR_RNDN);
						else mpfr_add(f_y1, f_y1, f_xi, MPFR_RNDN);
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
						mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						if(i % 2 == 0) mpfr_sub(f_y2, f_y2, f_xi, MPFR_RNDN);
						else mpfr_add(f_y2, f_y2, f_xi, MPFR_RNDN);
						mpfr_mul(f_y, f_y1, f_sin, MPFR_RNDN);
						mpfr_mul(f_yt, f_y2, f_cos, MPFR_RNDN);
						mpfr_sub(f_y, f_y, f_yt, MPFR_RNDN);
						mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
						if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
						mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
					}
					mpfr_clears(f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
				} else {
					mpfr_t f_euler;
					mpfr_inits2(mpfr_get_prec(i2 == 0 ? f_mid1 : f_mid2), f_x, f_xi, f_y, f_euler, NULL);
					mpfr_set(f_x, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpfr_const_euler(f_euler, MPFR_RNDN);
					mpfr_set(f_y, i2 == 0 ? f_mid1 : f_mid2, MPFR_RNDN);
					mpfr_log(f_y, f_y, MPFR_RNDN);
					mpfr_add(f_y, f_y, f_euler, MPFR_RNDN);
					mpz_set_ui(z_i, 0);
					mpz_set_ui(z_fac, 1);
					for(size_t i = 0; i < 10000; i++) {
						if(CALCULATOR->aborted()) {set(nr_bak); return false;}
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpz_add_ui(z_i, z_i, 1);
						mpz_mul(z_fac, z_fac, z_i);
						mpz_mul(z_fac, z_fac, z_i);
						mpfr_set(f_xi, f_x, MPFR_RNDN);
						mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
						mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
						mpz_divexact(z_fac, z_fac, z_i);
						if(i % 2 == 0) mpfr_sub(f_y, f_y, f_xi, MPFR_RNDN);
						else mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
						mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
						if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
						mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
					}
					mpfr_clear(f_euler);
				}
				if(b) {
					if(i2 == 0) {
						nr_mid1.setInternal(fl_value);
						mpfr_set(nr_mid1.internalUpperFloat(), f_y, MPFR_RNDU);
						if(!nr_mid1.testFloatResult()) b = false;
					} else {
						nr_mid2.setInternal(fl_value);
						mpfr_set(nr_mid2.internalUpperFloat(), f_y, MPFR_RNDU);
						if(!nr_mid2.testFloatResult()) b = false;
					}
				}
				mpfr_clears(f_x, f_xi, f_y, NULL);
				mpz_clears(z_i, z_fac, NULL);
			}
			mpfr_clears(f_pi, f_mid1, f_mid2, NULL);
		}
		if(!b || !nr_lower.cosint() || !nr_upper.cosint()) {
			set(nr_bak);
			return false;
		}
		setInterval(nr_lower, nr_upper);
		if(b1) setInterval(*this, nr_mid1);
		if(b2) setInterval(*this, nr_mid2);
		return true;
	}
	mpfr_clear_flags();
	mpfr_t f_x, f_xi, f_y;
	mpz_t z_i, z_fac;
	mpz_inits(z_i, z_fac, NULL);
	bool b = false;
	if(nr_round > 100 + PRECISION * 5) {
		mpfr_t f_sin, f_cos, f_y1, f_y2, f_yt;
		mpfr_inits2(mpfr_get_prec(fl_value) + 10, f_x, f_xi, f_y, NULL);
		mpfr_set(f_x, fl_value, MPFR_RNDN);
		mpfr_inits2(mpfr_get_prec(f_x), f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
		mpfr_set(f_cos, f_x, MPFR_RNDN);
		mpfr_set(f_sin, f_x, MPFR_RNDN);
		mpfr_cos(f_cos, f_cos, MPFR_RNDN);
		mpfr_div(f_cos, f_cos, f_x, MPFR_RNDN);
		mpfr_sin(f_sin, f_sin, MPFR_RNDN);
		mpfr_div(f_sin, f_sin, f_x, MPFR_RNDN);
		mpfr_set_ui(f_y1, 1, MPFR_RNDN);
		mpfr_ui_div(f_y2, 1, f_x, MPFR_RNDN);
		mpz_set_ui(z_i, 1);
		mpz_set_ui(z_fac, 1);
		for(size_t i = 0; i < 100; i++) {
			if(CALCULATOR->aborted()) {set(nr_bak); return false;}
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
			mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			if(i % 2 == 0) mpfr_sub(f_y1, f_y1, f_xi, MPFR_RNDN);
			else mpfr_add(f_y1, f_y1, f_xi, MPFR_RNDN);
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_ui_div(f_xi, 1, f_xi, MPFR_RNDN);
			mpfr_mul_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			if(i % 2 == 0) mpfr_sub(f_y2, f_y2, f_xi, MPFR_RNDN);
			else mpfr_add(f_y2, f_y2, f_xi, MPFR_RNDN);
			mpfr_mul(f_y, f_y1, f_sin, MPFR_RNDN);
			mpfr_mul(f_yt, f_y2, f_cos, MPFR_RNDN);
			mpfr_sub(f_y, f_y, f_yt, MPFR_RNDN);
			mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
			if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
			mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		}
		mpfr_clears(f_sin, f_cos, f_y1, f_y2, f_yt, NULL);
	} else {
		mpfr_t f_euler;
		mpfr_inits2(mpfr_get_prec(fl_value) + nr_round.intValue() * 5, f_x, f_xi, f_y, f_euler, NULL);
		mpfr_set(f_x, fl_value, MPFR_RNDN);
		mpfr_const_euler(f_euler, MPFR_RNDN);
		mpfr_set(f_y, fl_value, MPFR_RNDN);
		mpfr_log(f_y, f_y, MPFR_RNDN);
		mpfr_add(f_y, f_y, f_euler, MPFR_RNDN);
		mpz_set_ui(z_i, 0);
		mpz_set_ui(z_fac, 1);
		for(size_t i = 0; i < 10000; i++) {
			if(CALCULATOR->aborted()) {set(nr_bak); return false;}
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpz_add_ui(z_i, z_i, 1);
			mpz_mul(z_fac, z_fac, z_i);
			mpz_mul(z_fac, z_fac, z_i);
			mpfr_set(f_xi, f_x, MPFR_RNDN);
			mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
			mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
			mpz_divexact(z_fac, z_fac, z_i);
			if(i % 2 == 0) mpfr_sub(f_y, f_y, f_xi, MPFR_RNDN);
			else mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
			mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
			if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
			mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		}
		mpfr_clear(f_euler);
	}
	if(!b) {
		set(nr_bak);
		return false;
	}
	if(CREATE_INTERVAL) mpfr_set(fu_value, f_y, MPFR_RNDU);
	else mpfr_set(fu_value, fl_value, MPFR_RNDN);
	mpfr_clears(f_x, f_xi, f_y, NULL);
	mpz_clears(z_i, z_fac, NULL);
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;
}
bool Number::coshint() {
	if(isPlusInfinity()) {return true;}
	if(isZero()) {setMinusInfinity(true); return true;}
	if(hasImaginaryPart() && !hasRealPart()) {
		if(i_value->isNegative()) {
			set(*i_value, true);
			negate();
			if(!cosint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(-2);
			setImaginaryPart(nr_i);
			return true;
		} else if(i_value->isPositive()) {
			set(*i_value, true);
			if(!cosint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(2);
			setImaginaryPart(nr_i);
			return true;
		} else {
			set(*i_value, true);
			if(!cosint()) return false;
			Number nr_i;
			nr_i.pi();
			nr_i.divide(2);
			Number nr_low(nr_i);
			nr_low.negate();
			nr_i.setInterval(nr_low, nr_i);
			setImaginaryPart(nr_i);
			return true;
		}
	}
	if(!isReal()) return false;
	Number nr_bak(*this);
	if(isNegative()) {
		if(!negate() || !coshint() || !negate()) {set(nr_bak); return false;}
		if(!i_value) {i_value = new Number(); i_value->markAsImaginaryPart();}
		i_value->pi();
		setPrecisionAndApproximateFrom(*i_value);
		return true;
	}
	if(isGreaterThan(1000)) return false;
	if(!setToFloatingPoint()) return false;
	if(isInterval()) {
		Number nr_lower(lowerEndPoint());
		Number nr_upper(upperEndPoint());
		if(!nr_lower.coshint() || !nr_upper.coshint()) {
			set(nr_bak);
			return false;
		}
		if(!isNonZero()) {
			setInterval(nr_lower, nr_upper);
			setInterval(nr_minus_inf, *this);
		} else {
			setInterval(nr_lower, nr_upper);
		}
		return true;
	}
	mpfr_clear_flags();
	mpfr_t f_x, f_xi, f_y, f_euler;
	mpz_t z_i, z_fac;
	mpz_inits(z_i, z_fac, NULL);
	Number nr_round(*this);
	nr_round.round();
	mpfr_inits2(mpfr_get_prec(fl_value) + nr_round.intValue(), f_x, f_xi, f_y, f_euler, NULL);
	mpfr_set(f_x, fl_value, MPFR_RNDN);
	mpfr_const_euler(f_euler, MPFR_RNDN);
	mpfr_set(f_y, fl_value, MPFR_RNDN);
	mpfr_log(f_y, f_y, MPFR_RNDN);
	mpfr_add(f_y, f_y, f_euler, MPFR_RNDN);
	mpz_set_ui(z_i, 0);
	mpz_set_ui(z_fac, 1);
	bool b = false;
	for(size_t i = 0; i < 10000; i++) {
		if(CALCULATOR->aborted()) {set(nr_bak); return false;}
		mpz_add_ui(z_i, z_i, 1);
		mpz_mul(z_fac, z_fac, z_i);
		mpz_add_ui(z_i, z_i, 1);
		mpz_mul(z_fac, z_fac, z_i);
		mpz_mul(z_fac, z_fac, z_i);
		mpfr_set(f_xi, f_x, MPFR_RNDN);
		mpfr_pow_z(f_xi, f_xi, z_i, MPFR_RNDN);
		mpfr_div_z(f_xi, f_xi, z_fac, MPFR_RNDN);
		mpz_divexact(z_fac, z_fac, z_i);
		mpfr_add(f_y, f_y, f_xi, MPFR_RNDN);
		mpfr_set(fl_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
		if(i > 0 && mpfr_equal_p(fu_value, fl_value)) {b = true; break;}
		mpfr_set(fu_value, f_y, CREATE_INTERVAL ? MPFR_RNDD : MPFR_RNDN);
	}
	if(!b) {
		set(nr_bak);
		return false;
	}
	if(CREATE_INTERVAL) mpfr_set(fu_value, f_y, MPFR_RNDU);
	else mpfr_set(fu_value, fl_value, MPFR_RNDN);
	mpfr_clears(f_x, f_xi, f_y, f_euler, NULL);
	mpz_clears(z_i, z_fac, NULL);
	if(!testFloatResult()) {
		set(nr_bak);
		return false;
	}
	b_approx = true;
	return true;
}

bool recfact(mpz_ptr ret, long int start, long int n) {
	long int i;
	if(n <= 16) { 
		mpz_set_si(ret, start);
		for(i = start + 1; i < start + n; i++) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	if(!recfact(ret, start, i)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfact(retmul, start + i, n - i)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}
bool recfact2(mpz_ptr ret, long int start, long int n) {
	long int i;
	if(n <= 32) { 
		mpz_set_si(ret, start + n - 1);
		for(i = start + n - 3; i >= start; i -= 2) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	if(n % 2 != i % 2) i--;
	if(!recfact2(ret, start, i)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfact2(retmul, start + i, n - i)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}
bool recfactm(mpz_ptr ret, long int start, long int n, long int m) {
	long int i;
	if(n <= 16 * m) { 
		mpz_set_si(ret, start + n - 1);
		for(i = start + n - 1 - m; i >= start; i -= m) mpz_mul_si(ret, ret, i);
		return true;
	}
	if(CALCULATOR->aborted()) return false;
	i = n / 2;
	i -= ((i % m) - (n % m));
	if(!recfactm(ret, start, i, m)) return false;
	mpz_t retmul;
	mpz_init(retmul);
	if(!recfactm(retmul, start + i, n - i, m)) return false;
	mpz_mul(ret, ret, retmul);
	mpz_clear(retmul);
	return true;
}

bool Number::factorial() {
	if(!isInteger()) {
		return false;
	}
	if(isNegative()) {
		/*if(b_imag) return false;
		setPlusInfinity();
		return true;*/
		return false;
	}
	if(isZero()) {
		set(1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value))) return false;
	long int n = mpz_get_si(mpq_numref(r_value));
	if(!recfact(mpq_numref(r_value), 1, n)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::multiFactorial(const Number &o) {
	if(!isInteger() || !o.isInteger() || !o.isPositive()) {
		return false;
	}
	if(isZero()) {
		set(1, 1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value)) || !mpz_fits_slong_p(mpq_numref(o.internalRational()))) return false;
	long int n = mpz_get_si(mpq_numref(r_value));
	long int m = mpz_get_si(mpq_numref(o.internalRational()));
	if(!recfactm(mpq_numref(r_value), 1, n, m)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::doubleFactorial() {
	if(!isInteger()) {
		return false;
	}
	if(isZero() || isMinusOne()) {
		set(1, 1);
		return true;
	} else if(isOne()) {
		return true;
	} else if(isNegative()) {
		return false;
	}
	if(!mpz_fits_slong_p(mpq_numref(r_value))) return false;
	unsigned long int n = mpz_get_si(mpq_numref(r_value));
	if(!recfact2(mpq_numref(r_value), 1, n)) {
		mpz_set_si(mpq_numref(r_value), n);
		return false;
	}
	return true;
}
bool Number::binomial(const Number &m, const Number &k) {
	if(!m.isInteger() || !k.isInteger()) return false;
	if(m.isNegative()) {
		if(k.isNegative()) return false;
		Number m2(m);
		if(!m2.negate() || !m2.add(k) || !m2.add(nr_minus_one) || !binomial(m2, k)) return false;
		if(k.isOdd()) negate();
		return true;
	}
	if(k.isNegative() || k.isGreaterThan(m)) {
		clear();
		return true;
	}
	if(m.isZero() || m.equals(k)) {
		set(1, 1, 0);
		return true;
	}
	if(!mpz_fits_ulong_p(mpq_numref(k.internalRational()))) return false;
	clear();
	mpz_bin_ui(mpq_numref(r_value), mpq_numref(m.internalRational()), mpz_get_ui(mpq_numref(k.internalRational())));
	return true;
}

bool Number::factorize(vector<Number> &factors) {
	if(isZero() || !isInteger()) return false;
	if(mpz_cmp_si(mpq_numref(r_value), 1) == 0) {
		factors.push_back(nr_one);
		return true;
	}
	if(mpz_cmp_si(mpq_numref(r_value), -1) == 0) {
		factors.push_back(nr_minus_one);
		return true;
	}
	mpz_t inr, last_prime, facmax;
	mpz_inits(inr, last_prime, facmax, NULL);
	mpz_set(inr, mpq_numref(r_value));
	if(mpz_sgn(inr) < 0) {
		mpz_neg(inr, inr);
		factors.push_back(nr_minus_one);
	}
	size_t prime_index = 0;
	bool b = true;
	while(b) {
		if(CALCULATOR->aborted()) {mpz_clears(inr, last_prime, facmax, NULL); return false;}
		b = false;
		mpz_sqrt(facmax, inr);
		for(; prime_index < NR_OF_PRIMES && mpz_cmp_si(facmax, PRIMES[prime_index]) >= 0; prime_index++) {
			if(mpz_divisible_ui_p(inr, (unsigned long int) PRIMES[prime_index])) {
				mpz_divexact_ui(inr, inr, (unsigned long int) PRIMES[prime_index]);
				Number fac(PRIMES[prime_index], 1);;
				factors.push_back(fac);
				b = true;
				break;
			}
		}
		if(prime_index == NR_OF_PRIMES) {
			mpz_set_si(last_prime, PRIMES[NR_OF_PRIMES - 1] + 2);
			prime_index++;
		}
		if(!b && prime_index > NR_OF_PRIMES) {
			while(!b && mpz_cmp(facmax, last_prime) >= 0) {
				if(CALCULATOR->aborted()) {mpz_clears(inr, last_prime, facmax, NULL); return false;}
				if(mpz_divisible_p(inr, last_prime)) {
					mpz_divexact(inr, inr, last_prime);
					b = true;
					Number fac;
					fac.setInternal(last_prime);
					factors.push_back(fac);
					break;
				}
				mpz_add_ui(last_prime, last_prime, 2);
			}
		}
	}
	if(mpz_cmp_si(mpq_numref(r_value), 1) != 0) {
		Number fac;
		fac.setInternal(inr);
		factors.push_back(fac);
	}
	mpz_clears(inr, last_prime, facmax, NULL);
	return true;
}

void Number::rand() {
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_inits2(BIT_PRECISION, fl_value, fu_value, NULL);
		mpq_set_ui(r_value, 0, 1);
		n_type = NUMBER_TYPE_FLOAT;
	}
	mpfr_urandom(fu_value, randstate, MPFR_RNDN);
	mpfr_set(fl_value, fu_value, MPFR_RNDN);
	b_approx = false;
	i_precision = -1;
}
void Number::intRand(const Number &ceil) {
	clear();
	if(!ceil.isInteger() || !ceil.isPositive()) return;
	mpz_urandomm(mpq_numref(r_value), randstate, mpq_numref(ceil.internalRational()));
}

bool Number::add(const Number &o, MathOperation op) {
	switch(op) {
		case OPERATION_SUBTRACT: {
			return subtract(o);
		}
		case OPERATION_ADD: {
			return add(o);
		} 
		case OPERATION_MULTIPLY: {
			return multiply(o);
		}
		case OPERATION_DIVIDE: {
			return divide(o);
		}		
		case OPERATION_RAISE: {
			return raise(o);
		}
		case OPERATION_EXP10: {
			return exp10(o);
		}
		case OPERATION_BITWISE_AND: {
			return bitAnd(o);
		}
		case OPERATION_BITWISE_OR: {
			return bitOr(o);
		}
		case OPERATION_BITWISE_XOR: {
			return bitXor(o);
		}
		case OPERATION_LOGICAL_OR: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 >= COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_EQUAL_OR_GREATER) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 >= COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_EQUAL_OR_GREATER) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 >= COMPARISON_RESULT_UNKNOWN && !COMPARISON_IS_NOT_EQUAL(i2)) return false;
			if(i2 >= COMPARISON_RESULT_UNKNOWN && !COMPARISON_IS_NOT_EQUAL(i1)) return false;
			setTrue(COMPARISON_IS_NOT_EQUAL(i1) || COMPARISON_IS_NOT_EQUAL(i2));
			return true;
		}
		case OPERATION_LOGICAL_XOR: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 >= COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_EQUAL_OR_GREATER) return false;
			if(i2 >= COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_EQUAL_OR_GREATER) return false;
			if(COMPARISON_IS_NOT_EQUAL(i1)) setTrue(i2 == COMPARISON_RESULT_EQUAL);
			else setTrue(COMPARISON_IS_NOT_EQUAL(i2));
			return true;
		}
		case OPERATION_LOGICAL_AND: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 >= COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_EQUAL_OR_GREATER) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 >= COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_EQUAL_OR_GREATER) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 >= COMPARISON_RESULT_UNKNOWN && (i2 == COMPARISON_RESULT_UNKNOWN || COMPARISON_IS_NOT_EQUAL(i2))) return false;
			if(i2 >= COMPARISON_RESULT_UNKNOWN && COMPARISON_IS_NOT_EQUAL(i1)) return false;
			setTrue(COMPARISON_IS_NOT_EQUAL(i1) && COMPARISON_IS_NOT_EQUAL(i2));
			return true;
		}
		case OPERATION_EQUALS: {
			ComparisonResult i = compare(o);
			if(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
			setTrue(i == COMPARISON_RESULT_EQUAL);
			return true;
		}
		case OPERATION_GREATER: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_LESS: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_GREATER: {}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {}
				case COMPARISON_RESULT_EQUAL: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
		}
		case OPERATION_LESS: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_GREATER: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_LESS: {}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {}
				case COMPARISON_RESULT_EQUAL: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
		}
		case OPERATION_EQUALS_GREATER: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_EQUAL_OR_LESS: {}
				case COMPARISON_RESULT_EQUAL: {}
				case COMPARISON_RESULT_LESS: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_GREATER: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
			return false;
		}
		case OPERATION_EQUALS_LESS: {
			ComparisonResult i = compare(o);
			switch(i) {
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {}
				case COMPARISON_RESULT_EQUAL: {}
				case COMPARISON_RESULT_GREATER: {
					setTrue();
					return true;
				}
				case COMPARISON_RESULT_LESS: {
					setFalse();
					return true;
				}
				default: {
					return false;
				}
			}
			return false;
		}
		case OPERATION_NOT_EQUALS: {
			ComparisonResult i = compare(o);
			if(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
			setTrue(i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_GREATER || i == COMPARISON_RESULT_LESS);
			return true;
		}
	}
	return false;	
}
string Number::printNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return format_number_string(printMPZ(mpq_numref(r_value), base, false, lower_case), base, base_display, display_sign);
}
string Number::printDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return format_number_string(printMPZ(mpq_denref(r_value), base, false, lower_case), base, base_display, display_sign);
}
string Number::printImaginaryNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return format_number_string(printMPZ(mpq_numref(i_value ? i_value->internalRational() : nr_zero.internalRational()), base, false, lower_case), base, base_display, display_sign);
}
string Number::printImaginaryDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return format_number_string(printMPZ(mpq_denref(i_value ? i_value->internalRational() : nr_zero.internalRational()), base, false, lower_case), base, base_display, display_sign);
}

ostream& operator << (ostream &os, const Number &nr) {
	os << nr.print();
	return os;
}
string Number::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
	if(ips.minus) *ips.minus = false;
	if(ips.exp_minus) *ips.exp_minus = false;
	if(ips.num) *ips.num = "";
	if(ips.den) *ips.den = "";
	if(ips.exp) *ips.exp = "";
	if(ips.re) *ips.re = "";
	if(ips.im) *ips.im = "";
	if(ips.iexp) *ips.iexp = 0;
	if(po.is_approximate && isApproximate()) *po.is_approximate = true;
	if((po.base == BASE_SEXAGESIMAL || po.base == BASE_TIME) && isReal()) {
		Number nr(*this);
		bool neg = nr.isNegative();
		nr.setNegative(false);
		nr.trunc();
		PrintOptions po2 = po;
		po2.base = 10;
		po2.number_fraction_format = FRACTION_FRACTIONAL;
		string str = nr.print(po2);
		if(po.base == BASE_SEXAGESIMAL) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DEGREE, po.can_display_unicode_string_arg))) {
				str += SIGN_DEGREE;
			} else {
				str += "o";
			}	
		}
		nr = *this;
		nr.frac();
		nr *= 60;
		Number nr2(nr);
		nr.trunc();
		if(po.base == BASE_TIME) {
			str += ":";
			if(nr.isLessThan(10)) {
				str += "0";
			}
		}
		str += nr.printNumerator(10, false);
		if(po.base == BASE_SEXAGESIMAL) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("′", po.can_display_unicode_string_arg))) {
				str += "′";
			} else {
				str += "'";
			}
		}	
		nr2.frac();
		if(!nr2.isZero() || po.base == BASE_SEXAGESIMAL) {
			nr2.multiply(60);
			nr = nr2;
			nr.trunc();
			nr2.frac();
			if(!nr2.isZero()) {
				if(po.is_approximate) *po.is_approximate = true;
				if(nr2.isGreaterThanOrEqualTo(nr_half)) {
					nr.add(1);
				}
			}
			if(po.base == BASE_TIME) {
				str += ":";
				if(nr.isLessThan(10)) {
					str += "0";
				}
			}
			str += nr.printNumerator(10, false);
			if(po.base == BASE_SEXAGESIMAL) {
				if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("″", po.can_display_unicode_string_arg))) {
					str += "″";
				} else {
					str += "\"";
				}
			}
		}
		if(ips.minus) {
			*ips.minus = neg;
		} else if(neg) {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) str.insert(0, SIGN_MINUS);
			else str.insert(0, "-");
		}
		if(ips.num) *ips.num = str;
		
		return str;
	}
	string str;
	int base;
	long int min_decimals = 0;
	if(po.use_min_decimals && po.min_decimals > 0) min_decimals = po.min_decimals;
	if((int) min_decimals > po.max_decimals && po.use_max_decimals && po.max_decimals >= 0) {
		min_decimals = po.max_decimals;
	}
	if(po.base <= 1 && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_TIME) base = 10;
	else if(po.base > 36 && po.base != BASE_SEXAGESIMAL) base = 36;
	else base = po.base;
	if(po.base == BASE_ROMAN_NUMERALS) {
		if(!isRational()) {
			CALCULATOR->error(false, _("Can only display rational numbers as roman numerals."), NULL);
			base = 10;
		} else if(mpz_cmpabs_ui(mpq_numref(r_value), 9999) > 0 || mpz_cmp_ui(mpq_denref(r_value), 9999) > 0) {
			CALCULATOR->error(false, _("Cannot display numbers greater than 9999 or less than -9999 as roman numerals."), NULL);
			base = 10;
		}
	}
	
	if(hasImaginaryPart()) {
		if(i_value->isZero()) {
			Number nr;
			nr.set(*this, false, true);
			return nr.print(po, ips);
		}
		bool bre = hasRealPart();
		if(bre) {
			Number r_nr(*this);
			r_nr.clearImaginary();
			str = r_nr.print(po, ips);
			if(ips.re) *ips.re = str;
			InternalPrintStruct ips_n = ips;
			bool neg = false;
			ips_n.minus = &neg;
			string str2 = i_value->print(po, ips_n);
			if(ips.im) *ips.im = str2;
			if(!po.short_multiplication && str2 != "1") {
				if(po.spacious) {
					str2 += " * ";
				} else {
					str2 += "*";
				}
			}
			if(str2 == "1") str2 = "i";
			else str2 += "i";
			if(*ips_n.minus) {
				str += " - ";
			} else {
				str += " + ";
			}
			str += str2;
		} else {
			str = i_value->print(po, ips);
			if(ips.im) *ips.im = str;
			if(!po.short_multiplication && str != "1") {
				if(po.spacious) {
					str += " * ";
				} else {
					str += "*";
				}
			}
			if(str == "1") str = "i";
			else str += "i";
		}
		if(ips.num) *ips.num = str;
		return str;
	}

	long int precision = PRECISION;
	if(b_approx && i_precision >= 0 && (po.preserve_precision || po.preserve_format || i_precision < PRECISION)) precision = i_precision;
	else if(b_approx && i_precision < 0 && po.preserve_precision && FROM_BIT_PRECISION(BIT_PRECISION) > precision) precision = FROM_BIT_PRECISION(BIT_PRECISION);
	else if(b_approx && i_precision < 0 && po.preserve_format && FROM_BIT_PRECISION(BIT_PRECISION) - 1 > precision) precision = FROM_BIT_PRECISION(BIT_PRECISION) - 1;
	if(po.restrict_to_parent_precision && ips.parent_precision >= 0 && ips.parent_precision < precision) precision = ips.parent_precision;
	long int precision_base = precision;
	if(base != 10 && base >= 2 && base <= 36) {
		Number precmax(10);
		precmax.raise(precision_base);
		precmax--;
		precmax.log(base);
		precmax.floor();
		precision_base = precmax.lintValue();
	}
	long int i_precision_base = precision_base;
	if((i_precision < 0 && FROM_BIT_PRECISION(BIT_PRECISION) > precision) || i_precision > precision) {
		if(i_precision < 0) i_precision_base = FROM_BIT_PRECISION(BIT_PRECISION);
		else i_precision_base = i_precision;
		if(po.restrict_to_parent_precision && ips.parent_precision >= 0 && ips.parent_precision < i_precision_base) i_precision_base = ips.parent_precision;
		if(base != 10 && base >= 2 && base <= 36) {
			Number precmax(10);
			precmax.raise(i_precision_base);
			precmax--;
			precmax.log(base);
			precmax.floor();
			i_precision_base = precmax.lintValue();
		}
	}
	bool approx = isApproximate() || (ips.parent_approximate && po.restrict_to_parent_precision);

	if(isInteger()) {

		long int length = mpz_sizeinbase(mpq_numref(r_value), base);
		if(precision_base + min_decimals + 1000 + ::abs(po.min_exp) < length && ((approx || (base == 10 && po.min_exp != 0 && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT))) || length > (po.base == 10 ? 1000000L : 100000L))) {
			Number nr(*this);
			CALCULATOR->beginTemporaryStopMessages();
			PrintOptions po2 = po;
			po2.interval_display = INTERVAL_DISPLAY_MIDPOINT;
			if(nr.setToFloatingPoint()) {
				CALCULATOR->endTemporaryStopMessages(true);
				return nr.print(po2, ips);
			} else {
				length--;
				mpz_t ivalue;
				mpz_init(ivalue);
				mpz_ui_pow_ui(ivalue, base, length);
				Number nrexp;
				nrexp.setInternal(ivalue);
				if(nr.divide(nrexp)) {
					CALCULATOR->endTemporaryStopMessages();
					str = nr.print(po2, ips);
					if(base == 10) {
						if(ips.iexp) *ips.iexp = length;
						if(ips.exp) {
							if(ips.exp_minus) *ips.exp_minus = false;
							*ips.exp = i2s(length);
						} else {
							if(po.lower_case_e) str += "e";
							else str += "E";
							str += i2s(length);
						}
					} else {
						Number nrl(length);
						po2.twos_complement = false;
						po2.hexadecimal_twos_complement = false;
						po2.binary_bits = 0;
						string str_bexp = nrl.print(po2);
						if(ips.exp) {
							if(ips.exp_minus) *ips.exp_minus = false;
							*ips.exp = str_bexp;
						} else {
							if(po.spacious) str += " ";
							if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
							else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) str += SIGN_MULTIPLICATION;
							else str += "*";
							if(po.spacious) str += " ";
							str += i2s(base);
							str += "^";
							str += str_bexp;
							if(ips.depth > 0) {
								str.insert(0, "(");
								str += ")";
							}
						}
					}
					return str;
				}
				CALCULATOR->endTemporaryStopMessages(true);
				return "(floating point error)";
			}
		}

		if(po.base == 2 || (po.base == 16 && po.hexadecimal_twos_complement)) {
			if((po.base == 16 || po.twos_complement) && isNegative()) {
				Number nr;
				unsigned int bits = po.binary_bits;
				if(bits == 0) {
					nr = *this;
					nr.floor();
					nr++;
					bits = nr.integerLength() + 1;
					if(bits <= 8) bits = 8;
					else if(bits <= 16) bits = 16;
					else if(bits <= 32) bits = 32;
					else if(bits <= 64) bits = 64;
					else if(bits <= 128) bits = 128;
					else {
						bits = (unsigned int) ::ceil(::log2(bits));
						bits = ::pow(2, bits);
					}
				}
				nr = bits;
				nr.exp2();
				nr += *this;
				PrintOptions po2 = po;
				po2.twos_complement = false;
				if(!nr.isInteger() && po2.number_fraction_format == FRACTION_DECIMAL) {
					string str = print(po2);
					size_t i = str.find(po2.decimalpoint());
					if(i != string::npos) {
						po2.min_decimals = str.length() - (i + po2.decimalpoint().length());
						po2.max_decimals = po2.min_decimals;
						po2.use_max_decimals = true;
						po2.use_min_decimals = true;
					} else {
						po2.max_decimals = 0;
						po2.use_max_decimals = true;
					}
				}
				po2.binary_bits = bits;
				return nr.print(po2, ips);
			} else if(po.binary_bits == 0) {
				Number nr(*this);
				nr.ceil();
				unsigned int bits = nr.integerLength() + 1;
				if(bits <= 8) bits = 8;
				else if(bits <= 16) bits = 16;
				else if(bits <= 32) bits = 32;
				else if(bits <= 64) bits = 64;
				else if(bits <= 128) bits = 128;
				else {
					bits = (unsigned int) ::ceil(::log2(bits));
					bits = ::pow(2, bits);
				}
				PrintOptions po2 = po;
				po2.binary_bits = bits;
				return print(po2, ips);
			}
		}

		mpz_t ivalue;
		mpz_init_set(ivalue, mpq_numref(r_value));
		bool neg = (mpz_sgn(ivalue) < 0);
		bool rerun = false;
		bool exact = true;

		integer_rerun:

		string mpz_str = printMPZ(ivalue, base, false, base != BASE_ROMAN_NUMERALS && po.lower_case_numbers);

		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();

		length = mpz_str.length();
		long int expo = 0;
		if(base == 10 && !po.preserve_format) {
			if(length == 1 && mpz_str[0] == '0') {
				expo = 0;
			} else if(length > 0 && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				if(po.number_fraction_format == FRACTION_FRACTIONAL) {
					long int precexp = i_precision_base;
					if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
					else if(precexp > precision + 3) precexp = precision + 3;
					if(exact && ((expo >= 0 && length - 1 < precexp) || (expo < 0 && expo > -PRECISION))) expo = 0;
					else expo = length - 1;
				} else {
					expo = length - 1;
				}
			} else if(length > 0) {
				for(long int i = length - 1; i >= 0; i--) {
					if(mpz_str[i] != '0') {
						break;
					}
					expo++;
				}
			}
			if(po.min_exp == EXP_PRECISION) {
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if(exact && ((expo >= 0 && length - 1 < precexp) || (expo < 0 && expo > -PRECISION))) {
					if(precision_base < length) precision_base = length;
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				expo -= expo % (-po.min_exp);
				if(expo < 0) expo = 0;
			} else if(po.min_exp != 0) {
				if((long int) expo > -po.min_exp && (long int) expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		long int decimals = expo;
		long int nondecimals = length - decimals;

		bool dp_added = false;

		if(!rerun && mpz_sgn(ivalue) != 0) {
			long int precision2 = precision_base;
			if(min_decimals > 0 && min_decimals + nondecimals > precision_base) {
				precision2 = min_decimals + nondecimals;
				if(approx && precision2 > i_precision_base) precision2 = i_precision_base;
			}
			if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals + nondecimals < precision2) && (base == 10 && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT))) {
				mpz_t i_rem, i_quo, i_div;
				mpz_inits(i_rem, i_quo, i_div, NULL);
				mpz_ui_pow_ui(i_div, (unsigned long int) base, (unsigned long int) -(po.max_decimals - expo));
				mpz_fdiv_qr(i_quo, i_rem, ivalue, i_div);
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(ivalue, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), i_div);
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(ivalue)) {
						if(i_sign > 0) mpz_add_ui(ivalue, ivalue, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(ivalue, ivalue, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_mul(ivalue, ivalue, i_div);
					exact = false;
					rerun = true;
					mpz_clears(i_rem, i_quo, i_div, NULL);
					goto integer_rerun;
				}
				mpz_clears(i_rem, i_quo, i_div, NULL);
			} else if(precision2 < length && (approx || (base == 10 && expo != 0 && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)))) {

				mpq_t qvalue;
				mpq_init(qvalue);
				mpz_set(mpq_numref(qvalue), ivalue);
				
				precision2 = length - precision2;

				long int p2_cd = precision2;
				
				mpq_t q_exp;
				mpq_init(q_exp);

				long int p2_cd_min = 10000;
				while(p2_cd_min >= 1000) {
					if(p2_cd > p2_cd_min) {
						mpz_ui_pow_ui(mpq_numref(q_exp), (unsigned long int) base, (unsigned long int) p2_cd_min);
						while(p2_cd > p2_cd_min) {
							mpq_div(qvalue, qvalue, q_exp);
							p2_cd -= p2_cd_min;
							if(CALCULATOR->aborted()) {mpq_clears(q_exp, qvalue, NULL); mpz_clear(ivalue); return CALCULATOR->abortedMessage();}
						}
					}
					p2_cd_min = p2_cd_min / 10;
				}
				
				mpz_ui_pow_ui(mpq_numref(q_exp), (unsigned long int) base, (unsigned long int) p2_cd);
				mpq_div(qvalue, qvalue, q_exp);
					
				mpz_t i_rem, i_quo;
				mpz_inits(i_rem, i_quo, NULL);
				mpz_fdiv_qr(i_quo, i_rem, mpq_numref(qvalue), mpq_denref(qvalue));
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(ivalue, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), mpq_denref(qvalue));
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(ivalue)) {
						if(i_sign > 0) mpz_add_ui(ivalue, ivalue, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(ivalue, ivalue, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_ui_pow_ui(i_quo, (unsigned long int) base, (unsigned long int) precision2);
					mpz_mul(ivalue, ivalue, i_quo);
					exact = false;
					rerun = true;
					mpz_clears(i_rem, i_quo, NULL);
					mpq_clears(q_exp, qvalue, NULL);
					goto integer_rerun;
				}
				mpz_clears(i_rem, i_quo, NULL);
				mpq_clears(q_exp, qvalue, NULL);
			}
		}

		mpz_clear(ivalue);

		decimals = 0;
		if(expo > 0) {
			if(po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT) {
				mpz_str.insert(mpz_str.length() - expo, po.decimalpoint());
				dp_added = true;
				decimals = expo;
			} else {
				mpz_str = mpz_str.substr(0, mpz_str.length() - expo);
			}
		}
		
		if(base != BASE_ROMAN_NUMERALS && (po.restrict_fraction_length || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
			int pos = mpz_str.length() - 1;
			for(; pos >= (int) mpz_str.length() + min_decimals - decimals; pos--) {
				if(mpz_str[pos] != '0') {
					break;
				}
			}
			if(pos + 1 < (int) mpz_str.length()) {
				decimals -= mpz_str.length() - (pos + 1);
				mpz_str = mpz_str.substr(0, pos + 1);
			}
			if(exact && min_decimals > decimals) {
				if(decimals <= 0) {
					mpz_str += po.decimalpoint();
					dp_added = true;
				}
				while(min_decimals > decimals) {
					decimals++;
					mpz_str += "0";
				}
			}
			if(mpz_str[mpz_str.length() - 1] == po.decimalpoint()[0]) {
				mpz_str.erase(mpz_str.end() - 1);
				dp_added = false;
			}
		}

		if(!exact && po.is_approximate) *po.is_approximate = true;

		if(base != BASE_ROMAN_NUMERALS && po.show_ending_zeroes && (mpz_str.length() > 1 || mpz_str == "0") && (!exact || approx) && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals)) {
			precision = precision_base;
			precision -= mpz_str.length();
			if(dp_added) {
				precision += 1;
			} else if(precision > 0) {
				mpz_str += po.decimalpoint();
			}
			for(; precision > 0 && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals); precision--) {
				decimals++;
				mpz_str += "0";
			}
		}
		
		if(ips.minus) *ips.minus = neg;
		str = format_number_string(mpz_str, base, po.base_display, !ips.minus && neg, true, po);

		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				if(po.lower_case_e) str += "e";
				else str += "E";
				str += i2s(expo);
			}
		}
		if(ips.num) *ips.num = str;

	} else if(isPlusInfinity()) {
		str += "+";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
	} else if(isMinusInfinity()) {
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) str += SIGN_MINUS;
		else str += "-";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
	} else if(n_type == NUMBER_TYPE_FLOAT) {

		bool rerun = false;
		if(base < 2 || base > 36) base = 10;
		mpfr_clear_flags();
		
		mpfr_t f_diff, f_mid;
		
		bool is_interval = !mpfr_equal_p(fl_value, fu_value);

		if(!is_interval) {
			if(mpfr_inf_p(fl_value)) {
				Number nr;
				if(mpfr_sgn(fl_value) < 0) nr.setMinusInfinity();
				else nr.setPlusInfinity();
				nr.setPrecisionAndApproximateFrom(*this);
				return nr.print(po, ips);
			}
			mpfr_init2(f_mid, mpfr_get_prec(fl_value));
			mpfr_set(f_mid, fl_value, MPFR_RNDN);
		} else if(po.interval_display == INTERVAL_DISPLAY_INTERVAL) {
			PrintOptions po2 = po;
			InternalPrintStruct ips2;
			ips2.parent_approximate = ips.parent_approximate;
			ips2.parent_precision = ips.parent_precision;
			po2.interval_display = INTERVAL_DISPLAY_LOWER;
			string str1 = print(po2, ips2);
			po2.interval_display = INTERVAL_DISPLAY_UPPER;
			string str2 = print(po2, ips2);
			if(str1 == str2) return print(po2, ips);
			str = CALCULATOR->f_interval->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name;
			str += LEFT_PARENTHESIS;
			str += str1;
			str += po.comma();
			str += SPACE;
			str += str2;
			str += RIGHT_PARENTHESIS;
			if(ips.minus) *ips.minus = false;
			if(ips.num) *ips.num = str;
			return str;
		} else if(po.interval_display == INTERVAL_DISPLAY_PLUSMINUS) {
			if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
				PrintOptions po2 = po;
				po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
				return print(po2, ips);
			}
			mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
			mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
			mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
			mpfr_clear(f_diff);
			if(po.is_approximate) *po.is_approximate = true;
		} else if(po.interval_display == INTERVAL_DISPLAY_MIDPOINT) {
			if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
				PrintOptions po2 = po;
				po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
				return print(po2, ips);
			}
			mpfr_inits2(mpfr_get_prec(fl_value), f_diff, f_mid, NULL);
			mpfr_sub(f_diff, fu_value, fl_value, MPFR_RNDN);
			mpfr_div_ui(f_diff, f_diff, 2, MPFR_RNDN);
			mpfr_add(f_mid, fl_value, f_diff, MPFR_RNDN);
			mpfr_clear(f_diff);
			if(po.is_approximate) *po.is_approximate = true;
		} else if(po.interval_display == INTERVAL_DISPLAY_LOWER) {
			if(mpfr_inf_p(fl_value)) {
				Number nr;
				if(mpfr_sgn(fl_value) < 0) nr.setMinusInfinity();
				else nr.setPlusInfinity();
				nr.setPrecisionAndApproximateFrom(*this);
				return nr.print(po, ips);
			}
			mpfr_init2(f_mid, mpfr_get_prec(fl_value));
			mpfr_set(f_mid, fl_value, MPFR_RNDD);
		} else if(po.interval_display == INTERVAL_DISPLAY_UPPER) {
			if(mpfr_inf_p(fu_value)) {
				Number nr;
				if(mpfr_sgn(fu_value) < 0) nr.setMinusInfinity();
				else nr.setPlusInfinity();
				nr.setPrecisionAndApproximateFrom(*this);
				return nr.print(po, ips);
			}
			mpfr_init2(f_mid, mpfr_get_prec(fu_value));
			mpfr_set(f_mid, fu_value, MPFR_RNDU);
		} else {
			if(mpfr_inf_p(fl_value) || mpfr_inf_p(fu_value)) {
				PrintOptions po2 = po;
				po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
				return print(po2, ips);
			}
			mpfr_t vl, vu, f_logl, f_base, f_log_base;
			mpfr_inits2(mpfr_get_prec(fl_value), f_mid, vl, vu, f_logl, f_base, f_log_base, NULL);
		
			mpfr_set_si(f_base, base, MPFR_RNDN);
			mpfr_log(f_log_base, f_base, MPFR_RNDN);
			
			mpq_t base_half;
			mpq_init(base_half);
			mpq_set_ui(base_half, base, 2);
			mpq_canonicalize(base_half);
		
			if(mpfr_sgn(fl_value) != mpfr_sgn(fu_value)) {
				long int ilogl = i_precision_base, ilogu = i_precision_base;
				if(mpfr_sgn(fl_value) < 0) {
					mpfr_neg(f_logl, fl_value, MPFR_RNDU);
					mpfr_log(f_logl, f_logl, MPFR_RNDU);
					mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
					ilogl = -mpfr_get_si(f_logl, MPFR_RNDU);
					if(ilogl >= 0) {
						mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogl + 1, MPFR_RNDU);
						mpfr_mul(vl, fl_value, f_logl, MPFR_RNDD);
						mpfr_neg(vl, vl, MPFR_RNDU);
						if(mpfr_cmp_q(vl, base_half) <= 0) ilogl++;
					}
				}
				if(mpfr_sgn(fu_value) > 0) {
					mpfr_log(f_logl, fu_value, MPFR_RNDU);
					mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
					ilogu = -mpfr_get_si(f_logl, MPFR_RNDU);
					if(ilogu >= 0) {
						mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogu + 1, MPFR_RNDU);
						mpfr_mul(vu, fu_value, f_logl, MPFR_RNDU);
						if(mpfr_cmp_q(vu, base_half) <= 0) ilogu++;
					}
				}
				mpfr_clears(vu, vl, f_logl, f_mid, f_base, f_log_base, NULL);
				mpq_clear(base_half);
				if(ilogu < ilogl) ilogl = ilogu;
				if(ilogl <= 1) {
					PrintOptions po2 = po;
					po2.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
					return print(po2, ips);
				} else {
					i_precision_base = ilogl;
				}
				Number nr_zero;
				nr_zero.setApproximate(true);
				PrintOptions po2 = po;
				if(!po.use_max_decimals || po.max_decimals > i_precision_base - 1) {
					po2.max_decimals = i_precision_base - 1;
					po2.use_max_decimals = true;
				}
				return nr_zero.print(po2, ips);
			}
			
			float_interval_prec_rerun:
			
			mpfr_set(vl, fl_value, MPFR_RNDN);
			mpfr_set(vu, fu_value, MPFR_RNDN);
			bool negl = (mpfr_sgn(vl) < 0);
			if(negl) {
				mpfr_neg(vl, vl, MPFR_RNDN);
				mpfr_neg(vu, vu, MPFR_RNDN);
				mpfr_swap(vl, vu);
			}
			mpfr_nextbelow(vu);
			mpfr_log(f_logl, vu, MPFR_RNDN);
			mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDN);
			mpfr_floor(f_logl, f_logl);

			if(po.use_max_decimals && po.max_decimals >= 0) {
				long int i_log = mpfr_get_si(f_logl, MPFR_RNDN);
				long int expo = 0;
				if(!po.preserve_format && base == 10) {
					expo = i_log;
					if(po.min_exp == EXP_PRECISION || (po.min_exp == 0 && expo > 1000000L)) {
						long int precexp = i_precision_base;
						if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
						else if(precexp > precision + 3) precexp = precision + 3;
						if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
							if(expo >= i_precision_base) i_precision_base = expo + 1;
							if(expo >= precision_base) precision_base = expo + 1;
							if(expo >= precision) precision = expo + 1;
							expo = 0;
						}
					} else if(po.min_exp < -1) {
						if(expo < 0) {
							int expo_rem = (-expo) % (-po.min_exp);
							if(expo_rem > 0) expo_rem = (-po.min_exp) - expo_rem;
							expo -= expo_rem;
							if(expo > 0) expo = 0;
						} else if(expo > 0) {
							expo -= expo % (-po.min_exp);
							if(expo < 0) expo = 0;
						}
					} else if(po.min_exp != 0) {
						if(expo > -po.min_exp && expo < po.min_exp) { 
							expo = 0;
						}
					} else {
						expo = 0;
					}
				}
				if(i_log - expo + po.max_decimals < i_precision_base) i_precision_base = i_log - expo + po.max_decimals;
			}

			mpfr_sub_si(f_logl, f_logl, i_precision_base, MPFR_RNDN);
			mpfr_pow(f_logl, f_base, f_logl, MPFR_RNDN);
			mpfr_div(vl, vl, f_logl, MPFR_RNDU);
			mpfr_div(vu, vu, f_logl, MPFR_RNDD);
			if(mpfr_cmp(vl, vu) > 0) mpfr_swap(vl, vu);
			mpfr_round(vl, vl);
			mpfr_round(vu, vu);
			mpz_t ivalue;
			mpz_init(ivalue);
			mpfr_get_z(ivalue, vl, MPFR_RNDN);
			string str_l = printMPZ(ivalue, base, false, false);
			mpfr_get_z(ivalue, vu, MPFR_RNDN);
			string str_u = printMPZ(ivalue, base, false, false);

			if(str_u.length() > str_l.length()) {
				str_l.insert(0, str_u.length() - str_l.length(), '0');
			}
			for(size_t i = 0; i < str_l.size(); i++) {
				if(str_l[i] != str_u[i]) {
					if(char2val(str_l[i], base) + 1 == char2val(str_u[i], base)) {
						i++;
						bool do_rerun = false;
						for(; i < str_l.size(); i++) {
							if(char2val(str_l[i], base) == base - 1) {
								do_rerun = true;
							} else if(char2val(str_l[i], base) >= base / 2) {
								do_rerun = true;
								break;
							} else {
								i--;
								break;
							}
						}
						if(do_rerun) {
							if(i_precision_base == 0 || i == 0) {i_precision_base = 0; break;}
							else if(i > (size_t) i_precision_base) i_precision_base--;
							else i_precision_base = i - 1;
							goto float_interval_prec_rerun;
						}
					}
					if(i == 0 || i_precision_base <= 0) {
						if(i_precision_base < 0) break;
						mpfr_mul(f_mid, vu, f_logl, MPFR_RNDN);
						if(mpfr_cmp_ui(f_mid, 1) >= 0) {i_precision_base = 0; break;}
						i_precision_base = -1;
					} else if(i - 1 >= (size_t) i_precision_base) i_precision_base--;
					else i_precision_base = i - 1;
					goto float_interval_prec_rerun;
				} else if(i == str_l.size() - 1) {
					i_precision_base = i + 1;
				}
			}

			if(i_precision_base < precision_base) precision_base = i_precision_base;
			
			if(precision_base <= 1) {
				mpfr_clears(vu, vl, f_logl, f_mid, f_base, f_log_base, NULL);
				mpq_clear(base_half);
				PrintOptions po2 = po;
				po2.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
				return print(po2, ips);
			}
			
			if(i_precision_base <= 0) {
				if(negl) {
					mpfr_neg(vl, fl_value, MPFR_RNDN);
					mpfr_neg(vu, fu_value, MPFR_RNDN);
				} else {
					mpfr_set(vl, fl_value, MPFR_RNDN);
					mpfr_set(vu, fu_value, MPFR_RNDN);
				}
				mpfr_log(f_logl, vl, MPFR_RNDU);
				mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
				long int ilogl = -mpfr_get_si(f_logl, MPFR_RNDU);
				if(ilogl >= 0) {
					mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogl + 1, MPFR_RNDU);
					mpfr_mul(vl, fl_value, f_logl, MPFR_RNDD);
					mpfr_neg(vl, vl, MPFR_RNDU);
					if(mpfr_cmp_q(vl, base_half) <= 0) ilogl++;
				}
				mpfr_log(f_logl, vu, MPFR_RNDU);
				mpfr_div(f_logl, f_logl, f_log_base, MPFR_RNDU);
				long int ilogu = -mpfr_get_si(f_logl, MPFR_RNDU);
				if(ilogu >= 0) {
					mpfr_ui_pow_ui(f_logl, (unsigned long int) base, (unsigned long int) ilogu + 1, MPFR_RNDU);
					mpfr_mul(vu, fu_value, f_logl, MPFR_RNDU);
					if(mpfr_cmp_q(vu, base_half) <= 0) ilogu++;
				}
				mpfr_clears(vu, vl, f_logl, f_mid, f_base, f_log_base, NULL);
				mpq_clear(base_half);
				if(ilogu < ilogl) ilogl = ilogu;
				if(ilogl <= 0) {
					PrintOptions po2 = po;
					po2.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
					return print(po2, ips);
				} else {
					i_precision_base = ilogl;
				}
				Number nr_zero;
				nr_zero.setApproximate(true);
				PrintOptions po2 = po;
				if(!po.use_max_decimals || po.max_decimals > i_precision_base - 1) {
					po2.max_decimals = i_precision_base - 1;
					po2.use_max_decimals = true;
				}
				return nr_zero.print(po2, ips);
			}

			mpq_clear(base_half);
			
			if(precision_base <= 1) {
				mpfr_mul(f_mid, vu, f_logl, MPFR_RNDN);
				if(negl) mpfr_neg(f_mid, f_mid, MPFR_RNDN);
				if(mpfr_cmp_abs(f_mid, fu_value) > 0 || mpfr_cmp_abs(f_mid, fl_value) < 0) {
					mpfr_clears(f_mid, vl, vu, f_logl, f_base, f_log_base, NULL);
					PrintOptions po2 = po;
					po2.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
					return print(po2, ips);
				}
			} else {
				mpfr_sub(f_mid, fu_value, fl_value, MPFR_RNDN);
				mpfr_div_ui(f_mid, f_mid, 2, MPFR_RNDN);
				mpfr_add(f_mid, fl_value, f_mid, MPFR_RNDN);
			}

			mpfr_clears(vl, vu, f_logl, f_base, f_log_base, NULL);

			if(po.is_approximate) *po.is_approximate = true;
		}

		precision = precision_base;
		
		bool b_pm_zero = false;
		
		if(mpfr_zero_p(f_mid)) {
			if(po.interval_display == INTERVAL_DISPLAY_PLUSMINUS && is_interval) {
				mpfr_t f_lunc, f_unc;
				mpfr_inits2(mpfr_get_prec(f_mid), f_lunc, f_unc, NULL);
				mpfr_sub(f_lunc, f_mid, fl_value, MPFR_RNDU);
				mpfr_sub(f_unc, fu_value, f_mid, MPFR_RNDU);
				if(mpfr_cmp(f_lunc, f_unc) > 0) mpfr_swap(f_lunc, f_unc);
				if(!mpfr_zero_p(f_unc)) {
					b_pm_zero = true;
					mpfr_swap(f_unc, f_mid);
				}
				mpfr_clears(f_lunc, f_unc, NULL);
			}
			if(!b_pm_zero) {
				Number nr_zero;
				nr_zero.setApproximate(true);
				PrintOptions po2 = po;
				if(!po.use_max_decimals || po.max_decimals < i_precision_base - 1) {
					po2.max_decimals = i_precision_base - 1;
					po2.use_max_decimals = true;
				}
				mpfr_clear(f_mid);
				return nr_zero.print(po2, ips);
			}
		}
		
		bool use_max_idp = false;

		float_rerun:
		
		string str_bexp;
		bool neg_bexp = false;
		
		mpfr_t f_log, f_base, f_log_base;
		mpfr_inits2(mpfr_get_prec(f_mid), f_log, f_base, f_log_base, NULL);
		mpfr_set_si(f_base, base, MPFR_RNDN);
		mpfr_log(f_log_base, f_base, MPFR_RNDN);
		
		if(base != 10 && (mpfr_get_exp(f_mid) > 100000L || mpfr_get_exp(f_mid) < -100000L)) {
			bool b_neg = mpfr_sgn(f_mid) < 0;
			if(b_neg) mpfr_neg(f_mid, f_mid, MPFR_RNDN);
			if(base == 2) {
				mpfr_log2(f_log, f_mid, MPFR_RNDN);
			} else {
				mpfr_log(f_log, f_mid, MPFR_RNDN);
				mpfr_div(f_log, f_log, f_log_base, MPFR_RNDN);
			}
			mpfr_floor(f_log, f_log);
			mpz_t z_exp;
			mpz_init(z_exp);
			mpfr_get_z(z_exp, f_log, MPFR_RNDN);
			if(mpz_sgn(z_exp) < 0) {
				mpz_neg(z_exp, z_exp);
				neg_bexp = true;
			}
			Number nr_bexp;
			nr_bexp.setInternal(z_exp);
			PrintOptions po2 = po;
			po2.twos_complement = false;
			po2.hexadecimal_twos_complement = false;
			po2.binary_bits = 0;
			str_bexp = nr_bexp.print(po2);
			mpfr_ui_pow(f_log, base, f_log, MPFR_RNDN);
			mpfr_div(f_mid, f_mid, f_log, MPFR_RNDN);
			if(b_neg) mpfr_neg(f_mid, f_mid, MPFR_RNDN);
		}
		mpfr_t v;
		mpfr_init2(v, mpfr_get_prec(f_mid));
		long int expo = 0;
		long int l10 = 0;
		mpfr_set(v, f_mid, MPFR_RNDN);
		bool neg = (mpfr_sgn(v) < 0);
		if(neg) mpfr_neg(v, v, MPFR_RNDN);
		mpfr_log(f_log, v, MPFR_RNDN);
		mpfr_div(f_log, f_log, f_log_base, MPFR_RNDN);
		mpfr_floor(f_log, f_log);
		long int i_log = mpfr_get_si(f_log, MPFR_RNDN);
		if((base == 10 || (isInterval() && po.interval_display == INTERVAL_DISPLAY_MIDPOINT && i_log > 0 && i_log > precision)) && (!po.preserve_format || (is_interval && po.interval_display == INTERVAL_DISPLAY_PLUSMINUS))) {
			expo = i_log;
			if(po.min_exp == EXP_PRECISION || (po.min_exp == 0 && expo > 1000000L)) {
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
					if(expo >= i_precision_base) i_precision_base = expo + 1;
					if(expo >= precision_base) precision_base = expo + 1;
					if(expo >= precision) precision = expo + 1;
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				if(expo < 0) {
					int expo_rem = (-expo) % (-po.min_exp);
					if(expo_rem > 0) expo_rem = (-po.min_exp) - expo_rem;
					expo -= expo_rem;
					if(expo > 0) expo = 0;
				} else if(expo > 0) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				}
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		if(!rerun && i_precision_base > precision_base && min_decimals > 0 && (po.interval_display != INTERVAL_DISPLAY_PLUSMINUS || !is_interval)) {
			if(min_decimals > precision - 1 - (i_log - expo)) {
				precision = min_decimals + 1 + (i_log - expo);
				if(precision > i_precision_base) precision = i_precision_base;
				mpfr_clears(v, f_log, f_base, f_log_base, NULL);
				rerun = true;
				goto float_rerun;
			}
		}
		if(expo == 0 && i_log > precision) {
			precision = (i_precision_base > i_log + 1) ? i_log + 1 : i_precision_base;
		}
		mpfr_sub_si(f_log, f_log, ((use_max_idp || po.interval_display != INTERVAL_DISPLAY_PLUSMINUS || !is_interval) && po.use_max_decimals && po.max_decimals >= 0 && precision > po.max_decimals + i_log - expo) ? po.max_decimals + i_log - expo: precision - 1, MPFR_RNDN);
		l10 = expo - mpfr_get_si(f_log, MPFR_RNDN);
		mpfr_pow(f_log, f_base, f_log, MPFR_RNDN);
		if((!neg && po.interval_display == INTERVAL_DISPLAY_LOWER) || (neg && po.interval_display == INTERVAL_DISPLAY_UPPER)) {
			mpfr_div(v, v, f_log, MPFR_RNDU);
			mpfr_floor(v, v);
		} else if((neg && po.interval_display == INTERVAL_DISPLAY_LOWER) || (!neg && po.interval_display == INTERVAL_DISPLAY_UPPER)) {
			mpfr_div(v, v, f_log, MPFR_RNDD);
			mpfr_ceil(v, v);
		} else if(po.round_halfway_to_even) {
			mpfr_div(v, v, f_log, MPFR_RNDN);
			mpfr_rint(v, v, MPFR_RNDN);
		} else {
			mpfr_div(v, v, f_log, MPFR_RNDN);
			mpfr_round(v, v);
		}
		mpz_t ivalue;
		mpz_init(ivalue);
		mpfr_get_z(ivalue, v, MPFR_RNDN);

		str = printMPZ(ivalue, base, false, po.lower_case_numbers);

		bool show_ending_zeroes = po.show_ending_zeroes;
		
		string str_unc;

		if(b_pm_zero) {
			if(!rerun && !po.preserve_precision && l10 > 0 && str.length() > 2) {
				precision = str.length() - l10;
				if(precision < 2) precision = 2;
				mpfr_clears(v, f_log, f_base, f_log_base, NULL);
				mpz_clear(ivalue);
				rerun = true;
				goto float_rerun;
			}
			if(!po.preserve_precision) show_ending_zeroes = l10 > 0;
			str_unc = str;
			str = "0";
		} else if(po.interval_display == INTERVAL_DISPLAY_PLUSMINUS && is_interval) {
			mpfr_t f_lunc, f_unc;
			mpfr_inits2(mpfr_get_prec(f_mid), f_lunc, f_unc, NULL);
			mpfr_div(f_lunc, fl_value, f_log, MPFR_RNDD);
			mpfr_div(f_unc, fu_value, f_log, MPFR_RNDU);
			if(neg) mpfr_neg(v, v, MPFR_RNDN);
			mpfr_sub(f_lunc, v, f_lunc, MPFR_RNDU);
			mpfr_sub(f_unc, f_unc, v, MPFR_RNDU);
			if(mpfr_cmp(f_lunc, f_unc) > 0) mpfr_swap(f_lunc, f_unc);
			if(po.round_halfway_to_even) mpfr_rint(f_unc, f_unc, MPFR_RNDN);
			else mpfr_round(f_unc, f_unc);
			if(!mpfr_zero_p(f_unc)) {
				mpfr_get_z(ivalue, f_unc, MPFR_RNDN);
				str_unc = printMPZ(ivalue, base, false, po.lower_case_numbers);
				if(!po.preserve_precision) show_ending_zeroes = str.length() > str_unc.length() || precision == 2;
			}
			if(!rerun) {
				if(str_unc.length() > str.length()) {
					precision -= str_unc.length() - str.length();
					if(precision <= 0) {
						PrintOptions po2 = po;
						po2.interval_display = INTERVAL_DISPLAY_INTERVAL;
						mpfr_clears(f_mid, f_lunc, f_unc, v, f_log, f_base, f_log_base, NULL);
						mpz_clear(ivalue);
						return print(po2, ips);
					}
					use_max_idp = true;
					rerun = true;
					goto float_rerun;
				} else if(!po.preserve_precision && l10 > 0 && str_unc.length() > 2) {
					precision = str.length() - l10;
					if(precision < (long int) str.length() - (long int) str_unc.length() + 2) precision = str.length() - str_unc.length() + 2;
					mpfr_clears(f_lunc, f_unc, v, f_log, f_base, f_log_base, NULL);
					mpz_clear(ivalue);
					rerun = true;
					goto float_rerun;
				}
			}
		}

		if(l10 > 0) {
			if(!str_unc.empty()) {
				long int l10unc = str_unc.length() - l10;
				if(l10unc < 1) {
					str_unc.insert(str_unc.begin(), 1 - l10unc, '0');
					l10unc = 1;
				}
				str_unc.insert(l10unc, po.decimalpoint());
				int l2unc = 0;
				while(str_unc[str_unc.length() - 1 - l2unc] == '0') {
					l2unc++;
				}
				if(l2unc > 0 && !show_ending_zeroes) {
					if(min_decimals > 0) {
						int decimals = str_unc.length() - l10unc - 1;
						if(decimals - min_decimals < l2unc) l2unc = decimals - min_decimals;
					}
					if(l2unc > 0) str_unc = str_unc.substr(0, str_unc.length() - l2unc);
				}
				if(str_unc[str_unc.length() - 1] == po.decimalpoint()[0]) {
					str_unc.erase(str_unc.end() - 1);
				}
			}
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}
			str.insert(l10, po.decimalpoint());
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			if(l2 > 0 && !show_ending_zeroes) {
				if(min_decimals > 0) {
					int decimals = str.length() - l10 - 1;
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
			}
		} else if(l10 < 0) {
			while(l10 < 0) {
				l10++;
				str += "0";
				if(!str_unc.empty()) str_unc += "0";
			}
		}

		if(str.empty()) {
			str = "0";
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
		}

		if(!str_bexp.empty()) {
			PrintOptions po2 = po;
			po2.binary_bits = 0;
			str = format_number_string(str, base, po.base_display, !ips.minus && neg, true, po2);
			if(!str_unc.empty()) str_unc = format_number_string(str_unc, base, po.base_display, false, true, po2);
		} else {
			str = format_number_string(str, base, po.base_display, !ips.minus && neg, true, po);
			if(!str_unc.empty()) str_unc = format_number_string(str_unc, base, po.base_display, false, true, po);
		}
		
		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				
				if(!b_pm_zero) {
					if(base == 10) {
						if(po.lower_case_e) str += "e";
						else str += "E";
						str += i2s(expo);
					} else {
						if(po.spacious) str += " ";
						if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
						else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
						else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) str += SIGN_MULTIPLICATION;
						else str += "*";
						if(po.spacious) str += " ";
						str += i2s(base);
						str += "^";
						str += i2s(expo);
						if(ips.depth > 0) {
							str.insert(0, "(");
							str += ")";
						}
					}
				}
				if(!str_unc.empty()) {
					if(base == 10) {
						if(po.lower_case_e) str_unc += "e";
						else str_unc += "E";
						str_unc += i2s(expo);
					} else {
						if(po.spacious) str += " ";
						if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
						else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
						else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) str += SIGN_MULTIPLICATION;
						else str += "*";
						if(po.spacious) str += " ";
						str += i2s(base);
						str += "^";
						str += i2s(expo);
						if(ips.depth > 0) {
							str.insert(0, "(");
							str += ")";
						}
					}
				}
			}
		}

		if(!str_unc.empty()) {
			str += SIGN_PLUSMINUS;
			str += str_unc;
		}
		if(!str_bexp.empty()) {
			if(ips.exp) {
				if(ips.exp_minus) *ips.exp_minus = neg_bexp;
				else if(neg_bexp) str_bexp.insert(0, "-");
				*ips.exp = str_bexp;
			} else {
				if(po.spacious) str += " ";
				if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
				else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
				else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) str += SIGN_MULTIPLICATION;
				else str += "*";
				if(po.spacious) str += " ";
				str += i2s(base);
				str += "^";
				if(neg_bexp) {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) str += SIGN_MINUS;
					else str = "-";
				}
				str += str_bexp;
				if(ips.depth > 0) {
					str.insert(0, "(");
					str += ")";
				}
			}
		}
		if(ips.minus) *ips.minus = neg;
		if(ips.num) *ips.num = str;
		mpfr_clears(f_mid, v, f_log, f_base, f_log_base, NULL);
		mpz_clear(ivalue);
		if(po.is_approximate && mpfr_inexflag_p()) *po.is_approximate = true;
		testErrors(2);

	} else if(base != BASE_ROMAN_NUMERALS && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {

		long int numlength = mpz_sizeinbase(mpq_numref(r_value), base);
		long int denlength = mpz_sizeinbase(mpq_denref(r_value), base);
		if(precision_base + min_decimals + 1000 + ::abs(po.min_exp) < labs(numlength - denlength) && (approx || (po.min_exp != 0 && base == 10) || labs(numlength - denlength) > (po.base == 10 ? 1000000L : 100000L))) {
			long int length = labs(numlength - denlength);
			Number nr(*this);
			PrintOptions po2 = po;
			po2.interval_display = INTERVAL_DISPLAY_MIDPOINT;
			CALCULATOR->beginTemporaryStopMessages();
			if(nr.setToFloatingPoint()) {
				CALCULATOR->endTemporaryStopMessages(true);
				return nr.print(po2, ips);
			} else {
				mpz_t ivalue;
				mpz_init(ivalue);
				mpz_ui_pow_ui(ivalue, base, length);
				Number nrexp;
				nrexp.setInternal(ivalue);
				if(nr.divide(nrexp)) {
					CALCULATOR->endTemporaryStopMessages();
					str = nr.print(po2, ips);
					if(base == 10) {
						if(ips.iexp) *ips.iexp = length;
						if(ips.exp) {
							if(ips.exp_minus) *ips.exp_minus = false;
							*ips.exp = i2s(length);
						} else {
							if(po.lower_case_e) str += "e";
							else str += "E";
							str += i2s(length);
						}
					} else {
						Number nrl(length);
						po2.twos_complement = false;
						po2.hexadecimal_twos_complement = false;
						po2.binary_bits = 0;
						string str_bexp = nrl.print(po2);
						if(ips.exp) {
							if(ips.exp_minus) *ips.exp_minus = false;
							*ips.exp = str_bexp;
						} else {
							if(po.spacious) str += " ";
							if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) str += SIGN_MULTIDOT;
							else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) str += SIGN_MIDDLEDOT;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) str += SIGN_MULTIPLICATION;
							else str += "*";
							if(po.spacious) str += " ";
							str += i2s(base);
							str += "^";
							str += str_bexp;
							if(ips.depth > 0) {
								str.insert(0, "(");
								str += ")";
							}
						}
					}
					return str;
				}
				CALCULATOR->endTemporaryStopMessages(true);
				return "(floating point error)";
			}
		}

		mpz_t num, d, remainder, remainder2, exp;
		mpz_inits(num, d, remainder, remainder2, exp, NULL);
		mpz_set(d, mpq_denref(r_value));
		mpz_set(num, mpq_numref(r_value));
		bool neg = (mpz_sgn(num) < 0);
		if(neg) mpz_neg(num, num);
		mpz_tdiv_qr(num, remainder, num, d);
		bool exact = (mpz_sgn(remainder) == 0);
		vector<mpz_t*> remainders;
		bool started = false;
		long int expo = 0;
		long int precision2 = precision_base;
		int num_sign = mpz_sgn(num);
		long int min_l10 = 0, max_l10 = 0;
		bool applied_expo = false;
		bool try_infinite_series = po.indicate_infinite_series && po.number_fraction_format != FRACTION_DECIMAL_EXACT && !approx && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals >= 3);
		
		if(num_sign != 0) {

			str = printMPZ(num, base, false);

			if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
			
			long int length = str.length();
			if(base != 10 || po.preserve_format) {
				expo = 0;
			} else {
				expo = length - 1;
				if(po.min_exp == EXP_PRECISION) {
					long int precexp = i_precision_base;
					if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
					else if(precexp > precision + 3) precexp = precision + 3;
					if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
						if(expo >= precision) precision = expo + 1;
						if(expo >= precision2) precision2 = expo + 1;
						expo = 0;
					}
				} else if(po.min_exp < -1) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				} else if(po.min_exp != 0) {
					if(expo > -po.min_exp && expo < po.min_exp) { 
						expo = 0;
					}
				} else {
					expo = 0;
				}
			}
			long int decimals = expo;
			long int nondecimals = length - decimals;
			
			precision2 -= length;
			if(po.preserve_format) {
				precision2 += 100;
			} else if(!approx && min_decimals) {
				min_l10 = (min_decimals + nondecimals) - length;
				if(min_l10 > precision2) precision2 = min_l10;
			}
			
			int do_div = 0;
			
			if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals - decimals < precision2)) {
				do_div = 1;
			} else if(precision2 < 0 && (approx || decimals > min_decimals)) {
				do_div = 2;
			}
			if(try_infinite_series && expo > 0 && do_div != 2) {
				mpz_t i_div;
				mpz_init(i_div);
				mpz_ui_pow_ui(i_div, 10, expo);
				mpz_mul(d, d, i_div);
				mpz_set(num, mpq_numref(r_value));
				bool neg = (mpz_sgn(num) < 0);
				if(neg) mpz_neg(num, num);
				mpz_tdiv_qr(num, remainder, num, d);
				mpz_clear(i_div);
				exact = false;
				do_div = 0;
				precision2 = precision - nondecimals;
				if(min_decimals > 0) {
					if(precision2 < min_decimals) precision2 = min_decimals;
					min_l10 = min_decimals;
				}
				if(po.use_max_decimals && po.max_decimals >= 0) {
					if(precision2 > po.max_decimals) precision2 = po.max_decimals;
					max_l10 = po.max_decimals;
				}
				applied_expo = true;
			} else if(do_div) {
				mpz_t i_rem, i_quo, i_div, i_div_pre;
				mpz_inits(i_rem, i_quo, i_div, i_div_pre, NULL);
				mpz_ui_pow_ui(i_div_pre, (unsigned long int) base, do_div == 1 ? (unsigned long int) -(po.max_decimals - decimals) : (unsigned long int) -precision2);
				mpz_mul(i_div, i_div_pre, mpq_denref(r_value));
				mpz_fdiv_qr(i_quo, i_rem, mpq_numref(r_value), i_div);
				if(mpz_sgn(i_rem) != 0) {
					mpz_set(num, i_quo);
					mpq_t q_rem, q_base_half;
					mpq_inits(q_rem, q_base_half, NULL);
					mpz_set(mpq_numref(q_rem), i_rem);
					mpz_set(mpq_denref(q_rem), i_div);
					mpz_set_si(mpq_numref(q_base_half), base);
					mpq_mul(q_rem, q_rem, q_base_half);
					mpz_set_ui(mpq_denref(q_base_half), 2);
					int i_sign = mpq_cmp(q_rem, q_base_half);
					if(po.round_halfway_to_even && mpz_even_p(num)) {
						if(i_sign > 0) mpz_add_ui(num, num, 1);
					} else {
						if(i_sign >= 0) mpz_add_ui(num, num, 1);
					}
					mpq_clears(q_base_half, q_rem, NULL);
					mpz_mul(num, num, i_div_pre);
					exact = false;
					if(neg) mpz_neg(num, num);
				}
				mpz_clears(i_rem, i_quo, i_div, i_div_pre, NULL);
				mpz_set_ui(remainder, 0);
			}
			started = true;
			if(!applied_expo && po.use_max_decimals && po.max_decimals >= 0 && precision2 > po.max_decimals - decimals) precision2 = po.max_decimals - decimals;
		}

		bool b_bak = false;
		mpz_t remainder_bak, num_bak;
		if(num_sign == 0 && (try_infinite_series || (po.use_max_decimals && po.max_decimals >= 0) || min_decimals > 0)) {
			mpz_init_set(remainder_bak, remainder);
			mpz_init_set(num_bak, num);
			b_bak = true;
		}
		bool rerun = false;
		int first_rem_check = 0;
		
		rational_rerun:

		int infinite_series = 0;
		long int l10 = 0;
		if(rerun) {
			mpz_set(num, num_bak);
			mpz_set(remainder, remainder_bak);
		}

		if(po.preserve_format) precision2 += 100;
		long int prec2_begin = precision2;
		mpz_t *remcopy;
		while(!exact && precision2 > 0) {
			if(try_infinite_series) {
				remcopy = (mpz_t*) malloc(sizeof(mpz_t));
				mpz_init_set(*remcopy, remainder);
			}
			mpz_mul_si(remainder, remainder, base);
			mpz_tdiv_qr(remainder, remainder2, remainder, d);
			exact = (mpz_sgn(remainder2) == 0);
			if(!started) {
				started = (mpz_sgn(remainder) != 0);
			}
			if(started) {
				mpz_mul_si(num, num, base);
				mpz_add(num, num, remainder);
			}
			if(try_infinite_series) {
				if(started && first_rem_check == 0) {
					remainders.push_back(remcopy);
				} else {
					if(started) first_rem_check--;
					mpz_clear(*remcopy);
					free(remcopy);
				}
			}
			if(CALCULATOR->aborted()) {
				if(b_bak) mpz_clears(num_bak, remainder_bak, NULL); 
				mpz_clears(num, d, remainder, remainder2, exp, NULL); 
				return CALCULATOR->abortedMessage();
			}
			l10++;
			mpz_set(remainder, remainder2);
			if(try_infinite_series && !exact && started) {
				for(size_t i = 0; i < remainders.size(); i++) {
					if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
					if(!mpz_cmp(*remainders[i], remainder)) {
						infinite_series = remainders.size() - i;
						try_infinite_series = false;
						long int min_prec2_diff = precision2 - (prec2_begin - min_l10);
						long int max_prec2_diff = precision2 - (prec2_begin - max_l10);
						if(infinite_series == 1) precision2 = 3;
						else precision2 = infinite_series + 1;
						if(min_l10 > 0 && precision2 < min_prec2_diff) {
							precision2 = min_prec2_diff;
							if(max_l10 > 0 && precision2 > max_prec2_diff) precision2 = max_prec2_diff;
						} else if(max_l10 > 0 && precision2 > max_prec2_diff) {
							precision2 = max_prec2_diff;
							infinite_series = 0;
						}
						break;
					}
				}
			}
			if(started) {
				precision2--;
			}
		}

		for(size_t i = 0; i < remainders.size(); i++) {
			mpz_clear(*remainders[i]);
			free(remainders[i]);
		}
		remainders.clear();
		if(!exact && !infinite_series) {
			mpz_mul_si(remainder, remainder, base);
			mpz_tdiv_qr(remainder, remainder2, remainder, d);
			mpq_t q_rem, q_base_half;
			mpq_inits(q_rem, q_base_half, NULL);
			mpz_set(mpq_numref(q_rem), remainder);
			mpz_set_si(mpq_numref(q_base_half), base);
			mpz_set_ui(mpq_denref(q_base_half), 2);
			int i_sign = mpq_cmp(q_rem, q_base_half);
			if(po.round_halfway_to_even && mpz_sgn(remainder2) == 0 && mpz_even_p(num)) {
				if(i_sign > 0) mpz_add_ui(num, num, 1);
			} else {
				if(i_sign >= 0) mpz_add_ui(num, num, 1);
			}
			mpq_clears(q_base_half, q_rem, NULL);
		}
		if(!exact && !infinite_series && po.number_fraction_format == FRACTION_DECIMAL_EXACT && !approx) {
			PrintOptions po2 = po;
			po2.number_fraction_format = FRACTION_FRACTIONAL;
			po2.restrict_fraction_length = true;
			if(b_bak) mpz_clears(num_bak, remainder_bak, NULL); 
			mpz_clears(num, d, remainder, remainder2, exp, NULL);
			return print(po2, ips);
		}

		str = printMPZ(num, base, false, po.lower_case_numbers);
		if(base == 10 && !rerun && !po.preserve_format && !applied_expo) {
			expo = str.length() - l10 - 1;
			if(po.min_exp == EXP_PRECISION) {
				long int precexp = i_precision_base;
				if(precision < 8 && precexp > precision + 2) precexp = precision + 2;
				else if(precexp > precision + 3) precexp = precision + 3;
				if((expo > 0 && expo < precexp) || (expo < 0 && expo > -PRECISION)) {
					if(expo >= precision) precision = expo + 1;
					if(expo >= precision2) precision2 = expo + 1;
					expo = 0;
				}
			} else if(po.min_exp < -1) {
				if(expo < 0) {
					int expo_rem = (-expo) % (-po.min_exp);
					if(expo_rem > 0) expo_rem = (-po.min_exp) - expo_rem;
					expo -= expo_rem;
					if(expo > 0) expo = 0;
				} else if(expo > 0) {
					expo -= expo % (-po.min_exp);
					if(expo < 0) expo = 0;
				}
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
			if(expo < 0 && infinite_series > 0) {
				first_rem_check = str.length() - l10 - expo;
				precision2 = precision;
				try_infinite_series = true;
				if(min_decimals > 0) {
					min_l10 = min_decimals + first_rem_check;
					if(precision2 < min_l10) precision2 = min_l10;
				}
				if(po.use_max_decimals && po.max_decimals >= 0) {
					max_l10 = po.max_decimals + first_rem_check;
					if(precision2 > max_l10) precision2 = max_l10;
				}
				rerun = true;
				started = false;
				goto rational_rerun;
			}
		}

		if(!rerun && num_sign == 0 && expo <= 0 && po.use_max_decimals && po.max_decimals >= 0 && l10 + expo > po.max_decimals) {
			precision2 = po.max_decimals + (str.length() - l10 - expo);
			try_infinite_series = false;
			rerun = true;
			exact = false;
			started = false;
			goto rational_rerun;
		}
		if(!rerun && !approx && !exact && num_sign == 0 && expo <= 0 && min_decimals > 0 && l10 + expo < min_decimals) {
			precision2 = min_decimals + (str.length() - l10 - expo);
			rerun = true;
			started = false;
			try_infinite_series = po.indicate_infinite_series && po.number_fraction_format != FRACTION_DECIMAL_EXACT && !approx && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals >= 3);
			min_l10 = precision2;
			goto rational_rerun;
		}

		if(po.is_approximate && !exact && !infinite_series) *po.is_approximate = true;
		if(expo != 0 && !applied_expo) {
			l10 += expo;
		}
		while(l10 < 0) {
			str += '0';
			l10++;
		}
		if(b_bak) mpz_clears(num_bak, remainder_bak, NULL); 
		mpz_clears(num, d, remainder, remainder2, exp, NULL);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		if(l10 > 0) {
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}
			str.insert(l10, po.decimalpoint());
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			int decimals = str.length() - l10 - 1;
			if((!exact || approx) && !infinite_series && po.show_ending_zeroes && (int) str.length() - precision - 1 < l2) {
				l2 = str.length() - precision - 1;
				if(po.use_max_decimals && po.max_decimals >= 0 && decimals - l2 > po.max_decimals) {
					l2 = decimals - po.max_decimals;
				}
				while(l2 < 0) {
					l2++;
					str += '0';
				}
			}
			if(l2 > 0 && !infinite_series) {
				if(min_decimals > 0 && (!approx || (!po.show_ending_zeroes && (int) str.length() - precision - 1 < l2))) {
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
					if(approx && (int) str.length() - precision - 1 > l2) l2 = str.length() - precision - 1;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
			}
		}

		int decimals = 0;
		if(l10 > 0) {
			decimals = str.length() - l10 - 1;
		}

		if(str.empty()) {
			str = "0";
		}
		if(!exact && str == "0" && po.show_ending_zeroes && po.use_max_decimals && po.max_decimals >= 0 && po.max_decimals < precision) {
			str += po.decimalpoint();
			for(; decimals < po.max_decimals; decimals++) str += '0';
		}
		if(exact && min_decimals > decimals) {
			if(decimals <= 0) {
				str += po.decimalpoint();
				decimals = 0;
			}
			for(; decimals < min_decimals; decimals++) {
				str += "0";
			}
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
		}
		if(infinite_series) {
			size_t i_dp = str.find(po.decimalpoint());
			if(i_dp != string::npos && ((infinite_series == 1 && i_dp + po.decimalpoint().length() + 2 < str.length() - infinite_series) || (infinite_series > 1 && i_dp + po.decimalpoint().length() < str.length() - infinite_series))) {
#ifdef _WIN32
				str.insert(str.length() - (infinite_series == 1 ? 3 : infinite_series), " ");
#else
				if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (" ", po.can_display_unicode_string_arg))) str.insert(str.length() - (infinite_series == 1 ? 3 : infinite_series), " ");
				else str.insert(str.length() - (infinite_series == 1 ? 3 : infinite_series), " ");
#endif
			}
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) ("…", po.can_display_unicode_string_arg))) str += "…";
			else str += "...";
		}

		str = format_number_string(str, base, po.base_display, !ips.minus && neg, true, po);
		
		if(expo != 0) {
			if(ips.iexp) *ips.iexp = expo;
			if(ips.exp) {
				if(ips.exp_minus) {
					*ips.exp_minus = expo < 0;
					if(expo < 0) expo = -expo;
				}
				*ips.exp = i2s(expo);
			} else {
				if(po.lower_case_e) str += "e";
				else str += "E";
				str += i2s(expo);
			}
		}
		if(ips.minus) *ips.minus = neg;
		if(ips.num) *ips.num = str;
	} else {
		if(approx && po.show_ending_zeroes && base != BASE_ROMAN_NUMERALS) {
			PrintOptions po2 = po;
			po2.number_fraction_format = FRACTION_DECIMAL;
			return print(po2, ips);
		}
		Number num, den;
		num.setInternal(mpq_numref(r_value));
		den.setInternal(mpq_denref(r_value));
		if(isApproximate()) {
			num.setApproximate();
			den.setApproximate();
		}
		bool approximately_displayed = false;
		PrintOptions po2 = po;
		po2.is_approximate = &approximately_displayed;
		str = num.print(po2, ips);
		if(approximately_displayed && base != BASE_ROMAN_NUMERALS) {
			po2 = po;
			po2.number_fraction_format = FRACTION_DECIMAL;
			return print(po2, ips);
		}
		if(ips.num) *ips.num = str;
		if(po.spacious) str += " ";
		if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
			str += SIGN_DIVISION;
		} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
			str += SIGN_DIVISION_SLASH;
		} else {
			str += "/";
		}
		if(po.spacious) str += " ";
		InternalPrintStruct ips_n = ips;
		ips_n.minus = NULL;
		string str2 = den.print(po2, ips_n);
		if(approximately_displayed && base != BASE_ROMAN_NUMERALS) {
			po2 = po;
			po2.number_fraction_format = FRACTION_DECIMAL;
			return print(po2, ips);
		}
		if(ips.den) *ips.den = str2;
		str += str2;
		if(po.is_approximate && approximately_displayed) *po.is_approximate = true;
	}
	return str;
}


