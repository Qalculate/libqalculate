/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Number.h"
#include "Calculator.h"

#include <limits.h>
#include <sstream>
#include "util.h"

//default rounding mode
#define DRM MPFR_RNDN

#define REAL_PRECISION_FLOAT_RE(x)		cln::cl_float(cln::realpart(x), cln::float_format(PRECISION + 1))
#define REAL_PRECISION_FLOAT_IM(x)		cln::cl_float(cln::imagpart(x), cln::float_format(PRECISION + 1))
#define REAL_PRECISION_FLOAT(x)			cln::cl_float(x, cln::float_format(PRECISION + 1))
#define MIN_PRECISION_FLOAT_RE(x)		cln::cl_float(cln::realpart(x), cln::float_format(cln::float_format_lfloat_min + 1))
#define MIN_PRECISION_FLOAT_IM(x)		cln::cl_float(cln::imagpart(x), cln::float_format(cln::float_format_lfloat_min + 1))
#define MIN_PRECISION_FLOAT(x)			cln::cl_float(x, cln::float_format(cln::float_format_lfloat_min + 1))

#define CANNOT_FLOAT(x, p)			(((cln::plusp(cln::realpart(x)) && (cln::realpart(x) > cln::cl_float(cln::most_positive_float(cln::float_format(p)), cln::default_float_format) || cln::realpart(x) < cln::cl_float(cln::least_positive_float(cln::float_format(p)), cln::default_float_format))) || (cln::minusp(cln::realpart(x)) && (cln::realpart(x) < cln::cl_float(cln::most_negative_float(cln::float_format(p)), cln::default_float_format) || cln::realpart(x) > cln::cl_float(cln::least_negative_float(cln::float_format(p)), cln::default_float_format)))))

using namespace cln;

string format_number_string(string cl_str, int base, BaseDisplay base_display, bool show_neg, bool format_base_two = true) {
	if(format_base_two && base == 2 && base_display != BASE_DISPLAY_NONE) {
		int i2 = cl_str.length() % 4;
		if(i2 != 0) i2 = 4 - i2;
		if(base_display == BASE_DISPLAY_NORMAL) {
			for(int i = (int) cl_str.length() - 4; i > 0; i -= 4) {
				cl_str.insert(i, 1, ' ');
			}
		}
		for(; i2 > 0; i2--) {
			cl_str.insert(cl_str.begin(), 1, '0');
		}
	}	
	string str = "";
	if(show_neg) {
		str += '-';
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
	str += cl_str;
	return str;
}

string printMPZ(mpz_ptr integ_pre, int base = 10, bool display_sign = true, BaseDisplay base_display = BASE_DISPLAY_NORMAL, bool lower_case = false) {
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
	
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	if(sign == -1) {
		mpz_neg(integ, integ);
	}
	
	string cl_str;
	
	char *tmp = mpz_get_str(NULL, base, integ); 
	cl_str = tmp;
	void (*freefunc)(void *, size_t);
	mp_get_memory_functions (NULL, NULL, &freefunc);
	freefunc(tmp, strlen(tmp) + 1);
	
	if(lower_case) {
		for(size_t i = 0; i < cl_str.length(); i++) {
			if(cl_str[i] >= 'A' && cl_str[i] <= 'Z') {
				cl_str[i] += 32;
			}
		}
	}		
	if(cl_str[cl_str.length() - 1] == '.') {
		cl_str.erase(cl_str.length() - 1, 1);
	}
	
	mpz_clear(integ);
	
	return format_number_string(cl_str, base, base_display, sign == -1 && display_sign);
}
string printMPZ(mpz_srcptr integ_pre, int base = 10, bool display_sign = true, BaseDisplay base_display = BASE_DISPLAY_NORMAL, bool lower_case = false) {
	mpz_t integ;
	mpz_init_set(integ, integ_pre);
	string str = printMPZ(integ, base, display_sign, base_display, lower_case);
	mpz_clear(integ);
	return str;
}


Number::Number() {
	mpq_init(r_value);
	mpfr_init2(f_value, BIT_PRECISION);
	clear();
}
Number::Number(string number, const ParseOptions &po) {
	mpq_init(r_value);
	mpfr_init2(f_value, BIT_PRECISION);
	set(number, po);
}
Number::Number(long int numerator, long int denominator, long int exp_10) {
	mpq_init(r_value);
	mpfr_init2(f_value, BIT_PRECISION);
	set(numerator, denominator, exp_10);
}
Number::Number(const Number &o) {
	mpq_init(r_value);
	mpfr_init2(f_value, BIT_PRECISION);
	set(o);
}
Number::~Number() {
	mpq_clear(r_value);
	mpfr_clear(f_value);
}

void Number::set(string number, const ParseOptions &po) {

	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;

	if(po.base == BASE_ROMAN_NUMERALS) {
		remove_blanks(number);
		Number nr;
		Number cur;
		bool large = false;
		vector<Number> numbers;
		bool capital = false;
		for(size_t i = 0; i < number.length(); i++) {
			switch(number[i]) {
				case 'I': {
					if(!capital && i == number.length() - 1) {
						cur.set(2);
						CALCULATOR->error(false, _("Assuming the unusual practice of letting a last capital I mean 2 in a roman numeral."), NULL);
						break;
					}
				}
				case 'J': {capital = true;}
				case 'i': {}
				case 'j': {
					cur.set(1);
					break;
				}
				case 'V': {capital = true;}
				case 'v': {
					cur.set(5);
					break;
				}
				case 'X': {capital = true;}
				case 'x': {
					cur.set(10);
					break;
				}
				case 'L': {capital = true;}
				case 'l': {
					cur.set(50);
					break;
				}
				case 'C': {capital = true;}
				case 'c': {
					cur.set(100);
					break;
				}
				case 'D': {capital = true;}
				case 'd': {
					cur.set(500);
					break;
				}
				case 'M': {capital = true;}
				case 'm': {
					cur.set(1000);
					break;
				}
				case '(': {
					int multi = 1, multi2 = 0;
					bool turn = false;
					bool error = false;
					i++;
					for(; i < number.length(); i++) {
						if(number[i] == '|') {
							if(!turn) {
								turn = true;
								multi2 = multi;
							} else {
								error = true;
								break;
							}
						} else if(number[i] == ')') {
							if(turn) {
								multi2--;
								if(multi2 < 1) {
									break;
								}	
							} else {
								error = true;
								break;
							}
						} else if(number[i] == '(') {
							if(!turn) {
								multi++;	
							} else {
								error = true;
								break;
							}
						} else {
							error = true;
							i--;
							break;
						}
					}
					if(error | !turn) {
						CALCULATOR->error(true, _("Error in roman numerals: %s."), number.c_str(), NULL);
					} else {
						cur.set(10);
						cur.raise(multi);
						cur.multiply(100);
					}
					break;
				}
				case '|': {
					if(large) {
						cur.clear();
						large = false;
						break;
					} else if(number.length() > i + 1 && number[i + 2] == ')') {
						i++;
						int multi = 1;
						for(; i < number.length(); i++) {
							if(number[i] != ')') {
								i--;
								break;
							}
							multi++;
						}
						cur.set(10);
						cur.raise(multi);
						cur.multiply(50);
						break;
					} else if(number.length() > i + 2 && number[i + 2] == '|') {
						cur.clear();
						large = true;
						break;
					}
				}
				default: {
					cur.clear();
					CALCULATOR->error(true, _("Unknown roman numeral: %c."), number[i], NULL);
				}
			}
			if(!cur.isZero()) {
				if(large) {
					cur.multiply(100000);
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
		if(error) {
			PrintOptions pro;
			pro.base = BASE_ROMAN_NUMERALS;
			CALCULATOR->error(false, _("Errors in roman numerals: \"%s\". Interpreted as %s, which should be written as %s."), number.c_str(), nr.print().c_str(), nr.print(pro).c_str(), NULL);
		}
		values.clear();
		numbers.clear();
		set(nr);
		return;
	}
	mpz_t num, den;
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
	if(base > 36) base = 36;
	if(base < 0) base = 10;
	long int readprec = 0;
	bool numbers_started = false, minus = false, in_decimals = false, b_cplx = false, had_nonzero = false;
	for(size_t index = 0; index < number.size(); index++) {
		if(number[index] >= '0' && ((base >= 10 && number[index] <= '9') || (base < 10 && number[index] < '0' + base))) {
			mpz_mul_si(num, num, base);
			if(number[index] != '0') {
				mpz_add_ui(num, num, (unsigned long int) number[index] - '0');
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'a' && number[index] < 'a' + base - 10) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (unsigned long int) number[index] - 'a' + 10);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'A' && number[index] < 'A' + base - 10) {
			mpz_mul_si(num, num, base);
			mpz_add_ui(num, num, (unsigned long int) number[index] - 'A' + 10);
			if(in_decimals) {
				mpz_mul_si(den, den, base);
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(number[index] == 'E' && base <= 10) {
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
				mpz_clear(e_den);
			} else {
				mpz_t e_num;
				mpz_init(e_num);
				mpz_ui_pow_ui(e_num, 10, exp);
				mpz_mul(num, num, e_num);
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
				value = 0;
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
				return;
			}
		} else if(!numbers_started && number[index] == '-') {
			minus = !minus;
		} else if(number[index] == 'i') {
			b_cplx = true;
		} else if(number[index] != ' ') {
			CALCULATOR->error(true, _("Character \'%c\' was ignored in the number \"%s\" with base %s."), number[index], number.c_str(), i2s(base).c_str(), NULL);
		}
	}
	if(minus) mpz_neg(num, num);
	if(b_cplx) {
		//value = cln::complex(0, num / den);
	} else {
		mpz_set(mpq_numref(r_value), num);
		mpz_set(mpq_denref(r_value), den);
		mpq_canonicalize(r_value);
		n_type = NUMBER_TYPE_RATIONAL;
	}
	mpz_clears(num, den, NULL);
	if(po.read_precision == ALWAYS_READ_PRECISION || (in_decimals && po.read_precision == READ_PRECISION_WHEN_DECIMALS)) {
		if(base != 10) {
			Number precmax(10);
			precmax.raise(readprec);
			precmax--;
			precmax.log(base);
			precmax.floor();
			readprec = precmax.intValue();
		}
		i_precision = readprec;
		b_approx = true;
	} else {
		i_precision = -1;
	}

}
void Number::set(long int numerator, long int denominator, long int exp_10) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	i_precision = -1;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, numerator, denominator == 0 ? 1 : denominator);
	mpq_canonicalize(r_value);
	if(exp_10 != 0) {
		exp10(exp_10);
	}
}
void Number::setFloat(double d_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = true;
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_set_d(f_value, d_value, DRM);
	i_precision = 8;
}
void Number::setInternal(mpz_srcptr mpz_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	mpq_set_z(r_value, mpz_value);
	n_type = NUMBER_TYPE_RATIONAL;
	i_precision = -1;
}
void Number::setInternal(const mpz_t &mpz_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	mpq_set_z(r_value, mpz_value);
	n_type = NUMBER_TYPE_RATIONAL;
	i_precision = -1;
}
void Number::setInternal(const mpq_t &mpq_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	mpq_set(r_value, mpq_value);
	n_type = NUMBER_TYPE_RATIONAL;
	i_precision = -1;
}
void Number::setInternal(const mpz_t &mpz_num, const mpz_t &mpz_den) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	mpz_set(mpq_numref(r_value), mpz_num);
	mpz_set(mpq_denref(r_value), mpz_den);
	n_type = NUMBER_TYPE_RATIONAL;
	i_precision = -1;
}
void Number::setInternal(const mpfr_t &mpfr_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = true;
	n_type = NUMBER_TYPE_FLOAT;
	if(mpfr_get_prec(mpfr_value) != mpfr_get_prec(f_value)) mpfr_set_prec(f_value, mpfr_get_prec(mpfr_value));
	mpfr_set(f_value, mpfr_value, DRM);
	i_precision = (int) mpfr_get_prec(f_value);
}

void Number::setImaginaryPart(const Number &o) {
	//value = cln::complex(cln::realpart(value), cln::realpart(o.internalNumber()));
	testApproximate();
}
void Number::setImaginaryPart(long int numerator, long int denominator, long int exp_10) {
	Number o(numerator, denominator, exp_10);
	setImaginaryPart(o);
}
void Number::set(const Number &o) {
	b_inf = o.isInfinity(); 
	b_pinf = o.isPlusInfinity(); 
	b_minf = o.isMinusInfinity();
	mpq_set(r_value, o.internalRational());
	mpfr_set(f_value, o.internalFloat(), DRM);
	n_type = o.internalType();
	b_approx = o.isApproximate();
	i_precision = o.precision();
}
void Number::setInfinity() {
	b_approx = false;
	n_type = NUMBER_TYPE_INFINITY;
	i_precision = -1;
}
void Number::setPlusInfinity() {
	b_approx = false;
	n_type = NUMBER_TYPE_PLUS_INFINITY;
	i_precision = -1;
}
void Number::setMinusInfinity() {
	b_approx = false;
	n_type = NUMBER_TYPE_MINUS_INFINITY;
	i_precision = -1;
}

void Number::clear() {
	b_approx = false;
	n_type = NUMBER_TYPE_RATIONAL;
	mpq_set_si(r_value, 0, 1);
	i_precision = -1;
}

const mpq_t &Number::internalRational() const {
	return r_value;
}
const mpfr_t &Number::internalFloat() const {
	return f_value;
}
const NumberType &Number::internalType() const {
	return n_type;
}

const cl_N &Number::internalNumber() const {
	return value;
}

double Number::floatValue() const {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		return mpq_get_d(r_value);
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		return mpfr_get_d(f_value, DRM);
	} 
	return 0.0;
}
long int Number::intValue(bool *overflow) const {
	if(isInteger()) {
		if(mpz_fits_slong_p(mpq_numref(r_value)) == 0) {
			if(overflow) *overflow = true;
			if(mpz_sgn(mpq_numref(r_value)) == -1) return LONG_MIN;
			return LONG_MAX;	
		}
		return (int) mpz_get_si(mpq_numref(r_value));
	} else {
		Number nr(*this);
		nr.round();
		return nr.intValue(overflow);
	}
}

bool Number::isApproximate() const {
	return b_approx || isApproximateType();
}
bool Number::isApproximateType() const {
	return (n_type == NUMBER_TYPE_FLOAT);
}
void Number::setApproximate(bool is_approximate) {
	if(is_approximate != isApproximate()) {
		if(is_approximate) {
			i_precision = PRECISION;
			b_approx = true;
		} else {
			i_precision = -1;
			b_approx = false;
		}
	}
}

int Number::precision() const {
	return i_precision;
}
void Number::setPrecision(long int prec) {
	i_precision = prec;
	if(i_precision > 0) b_approx = true;
}

bool Number::isUndefined() const {
	return false;
}
bool Number::isInfinite() const {
	return n_type >= NUMBER_TYPE_INFINITY;
}
bool Number::isInfinity() const {
	return n_type == NUMBER_TYPE_INFINITY;
}
bool Number::isPlusInfinity() const {
	return n_type == NUMBER_TYPE_PLUS_INFINITY;
}
bool Number::isMinusInfinity() const {
	return n_type == NUMBER_TYPE_MINUS_INFINITY;
}

Number Number::realPart() const {
	if(isInfinite()) return *this;
	Number real_part;
	if(n_type == NUMBER_TYPE_RATIONAL) real_part.setInternal(r_value);
	else real_part.setInternal(f_value);
	return real_part;
}
Number Number::imaginaryPart() const {
	if(isInfinite()) return Number();
	Number imag_part;
	//imag_part.setInternal(cln::imagpart(value));
	return imag_part;
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
	//num.setInternal(cln::numerator(cln::rational(cln::imagpart(value))));
	return num;
}
Number Number::complexDenominator() const {
	Number den;
	//den.setInternal(cln::denominator(cln::rational(cln::imagpart(value))));
	return den;
}

void Number::operator = (const Number &o) {set(o);}
void Number::operator -- (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_sub_ui(f_value, f_value, 1, DRM);
	}
}
void Number::operator ++ (int) {
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_add_ui(f_value, f_value, 1, DRM);
	}
}
Number Number::operator - () const {Number o(*this); o.negate(); return o;}
Number Number::operator * (const Number &o) const {Number o2(*this); o2.multiply(o); return o2;}
Number Number::operator / (const Number &o) const {Number o2(*this); o2.divide(o); return o2;}
Number Number::operator + (const Number &o) const {Number o2(*this); o2.add(o); return o2;}
Number Number::operator - (const Number &o) const {Number o2(*this); o2.subtract(o); return o2;}
Number Number::operator ^ (const Number &o) const {Number o2(*this); o2.raise(o); return o2;}
Number Number::operator && (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_AND); return o2;}
Number Number::operator || (const Number &o) const {Number o2(*this); o2.add(o, OPERATION_LOGICAL_OR); return o2;}
Number Number::operator ! () const {Number o(*this); o.setLogicalNot(); return o;}
		
void Number::operator *= (const Number &o) {multiply(o);}
void Number::operator /= (const Number &o) {divide(o);}
void Number::operator += (const Number &o) {add(o);}
void Number::operator -= (const Number &o) {subtract(o);}
void Number::operator ^= (const Number &o) {raise(o);}
	
bool Number::operator == (const Number &o) const {return equals(o);}
bool Number::operator != (const Number &o) const {return !equals(o);}

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
	long int y = o.intValue(&overflow);
	if(overflow) return false;
	mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftRight(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	bool overflow = false;
	long int y = o.intValue(&overflow);
	if(overflow) return false;
	mpz_tdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shift(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	bool overflow = false;
	long int y = o.intValue(&overflow);
	if(overflow) return false;
	if(y < 0) mpz_tdiv_q_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) -y);
	else mpz_mul_2exp(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) y);
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::hasRealPart() const {
	if(isInfinite()) return true;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_sgn(r_value) != 0;
	return !mpfr_zero_p(f_value);
}
bool Number::hasImaginaryPart() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = !cln::zerop(cln::imagpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}
	return b;
}
void Number::removeFloatZeroPart() {
	if(!isInfinite() && isApproximateType() && !cln::zerop(cln::imagpart(value))) {
		if(PRECISION < cln::float_format_lfloat_min) {
			cl_F f_value = MIN_PRECISION_FLOAT_RE(value) + MIN_PRECISION_FLOAT_IM(value);
			if(MIN_PRECISION_FLOAT(f_value) == MIN_PRECISION_FLOAT_RE(value)) {
				value = cln::realpart(value);
			} else if(MIN_PRECISION_FLOAT(f_value) == MIN_PRECISION_FLOAT_IM(value)) {
				value = cln::complex(0, cln::imagpart(value));
			}
		} else {
			cl_F f_value = REAL_PRECISION_FLOAT_RE(value) + REAL_PRECISION_FLOAT_IM(value);
			if(REAL_PRECISION_FLOAT(f_value) == REAL_PRECISION_FLOAT_RE(value)) {
				value = cln::realpart(value);
			} else if(REAL_PRECISION_FLOAT(f_value) == REAL_PRECISION_FLOAT_IM(value)) {
				value = cln::complex(0, cln::imagpart(value));
			}
		}
	}
}
void Number::testApproximate() {
	if(!b_approx && isApproximateType()) {
		i_precision = PRECISION;
		b_approx = true;
	}
}
void Number::testInteger() {
	if(isApproximateType() && !isInfinite() && !isComplex()) {
		if(mpfr_integer_p(f_value)) {
			mpfr_get_z(mpq_numref(r_value), f_value, DRM);
			n_type = NUMBER_TYPE_RATIONAL;
		}
		bool b = false;
		try {
			if(PRECISION < cln::float_format_lfloat_min) {
				b = cln::zerop(cln::truncate2(MIN_PRECISION_FLOAT_RE(value)).remainder);
			} else {
				b = cln::zerop(cln::truncate2(REAL_PRECISION_FLOAT_RE(value)).remainder);
			}
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		}
		if(b) {
			cln::cl_N new_value;
			try {
				new_value = cln::round1(cln::realpart(value));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
				return;
			}
			value = new_value;
		}
	}
}
void Number::setPrecisionAndApproximateFrom(const Number &o) {
	if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
	if(o.isApproximate()) b_approx = true;
}

bool Number::isComplex() const {
	return hasImaginaryPart();
}
Number Number::integer() const {
	Number nr(*this);
	nr.round();
	return nr;
}
bool Number::isInteger() const {
	if(isInfinite()) return false;
	if(isComplex()) return false;
	if(isApproximateType()) return false;
	return mpz_cmp_ui(mpq_denref(r_value), 1) == 0;
}
bool Number::isRational() const {
	return !isInfinite() && !isComplex() && !isApproximateType();
}
bool Number::isReal() const {
	return !isInfinite() && !isComplex();
}
bool Number::isFraction() const {
	if(isInfinite()) return false;
	if(!isComplex()) {
		return mpz_cmpabs(mpq_denref(r_value), mpq_numref(r_value)) == 1;
	}
	return false; 
}
bool Number::isZero() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_zero_p(f_value);
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isOne() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpz_cmp(mpq_denref(r_value), mpq_numref(r_value)) == 0;
	return false;
}
bool Number::isTwo() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_cmp_si(r_value, 2, 1) == 0;
	return false;
}
bool Number::isTen() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_cmp_si(r_value, 10, 1) == 0;
	return false;
}
bool Number::isI() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = cln::zerop(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}
	return b && cln::imagpart(value) == 1;
}
bool Number::isMinusOne() const {
	if(n_type == NUMBER_TYPE_RATIONAL) return mpq_cmp_si(r_value, -1, 1) == 0;
	return false;
}
bool Number::isMinusI() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = cln::zerop(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}
	return b && cln::imagpart(value) == -1;
}
bool Number::isNegative() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::isNonNegative() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) > 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) >= 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isPositive() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) > 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::isNonPositive() const {
	if(isComplex()) return false;
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) <= 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) <= 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsNegative() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) < 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) < 0;
	else if(n_type == NUMBER_TYPE_MINUS_INFINITY) return true;
	return false;
}
bool Number::realPartIsPositive() const {
	if(n_type == NUMBER_TYPE_FLOAT) return mpfr_sgn(f_value) > 0;
	else if(n_type == NUMBER_TYPE_RATIONAL) return mpz_sgn(mpq_numref(r_value)) > 0;
	else if(n_type == NUMBER_TYPE_PLUS_INFINITY) return true;
	return false;
}
bool Number::imaginaryPartIsNegative() const {
	return !isInfinite() && cln::minusp(cln::imagpart(value));
}
bool Number::imaginaryPartIsPositive() const {
	return !isInfinite() && cln::plusp(cln::imagpart(value));
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
	if(isZero()) return true;
	return false;
}
bool Number::equals(const Number &o) const {
	if(b_inf) return false;
	if(b_pinf) return false;
	if(b_minf) return false;
	if(o.isInfinite()) return false;
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalFloat(), r_value) == 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) return mpfr_equal_p(f_value, o.internalFloat());
		else return mpfr_cmp_q(f_value, o.internalRational()) == 0;
	}
	return mpq_cmp(r_value, o.internalRational()) == 0;
}
bool Number::equalsApproximately(const Number &o, int prec) const {
	if(b_inf) return false;
	if(b_pinf) return false;
	if(b_minf) return false;
	if(o.isInfinite()) return false;
	if(value == o.internalNumber()) return true;
	if(isComplex() != o.isComplex()) return false;
	if(isComplex()) {
		return realPart().equalsApproximately(o.realPart(), prec) && imaginaryPart().equalsApproximately(o.imaginaryPart(), prec);
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
	/*if(isApproximateType() && o.isApproximateType()) {
		if(prec >= cln::float_format_lfloat_min) {
			return cln::cl_float(cln::realpart(value), cln::float_format(prec + 1)) == cln::cl_float(cln::realpart(o.internalNumber()), cln::float_format(prec + 1));
		}
		string str1, str2;
		std::ostringstream s1, s2;
		s1 << value;
		s2 << o.internalNumber();
		str1 = s1.str();
		str2 = s2.str();
		for(size_t i = str1.length() - 1, i2 = str2.length() - 1; ; i--, i2--) {
			if(is_not_in("0123456789.", str1[i])) {
				if(is_in("0123456789.", str2[i2])) return false;
				break;
			}
			if(is_not_in("0123456789.", str2[i2])) {
				if(is_in("0123456789.", str1[i])) return false;
				break;
			}
			if(str1[i] != str2[i2]) return false;
		}
		for(size_t i = 0; i < str1.length() && i < str2.length() && (int) i <= prec; i++) {
			if(str1[i] != str2[i]) return false;
			if(str1[i] == '.') prec++;
		}
		return true;
	}*/
	try {
		if(prec_choosen || isApproximate() || o.isApproximate()) {
			cln::cl_R diff = cln::realpart(value) - cln::realpart(o.internalNumber());
			if(cln::zerop(diff)) return true;
			cln::cl_R val;
			bool first_greatest = true;
			if(diff < 0) {
				first_greatest = false;
				diff = -diff;
				val = cln::realpart(o.internalNumber());
			} else {
				val = cln::realpart(value);
			}		
			val = val / diff;
			cln::cl_I precval = cln::expt_pos(cln::cl_I(10), cln::cl_I(prec));
			val = val / precval;
			if(val > 3) return true;
			cl_RA tenth = 1;
			tenth = tenth / 10;
			if(val < tenth) return false;
		
			cln::cl_R remainder1, remainder2;
			if(first_greatest) {
				remainder1 = cln::realpart(value);
				remainder2 = cln::realpart(o.internalNumber());
			} else {
				remainder2 = cln::realpart(value);
				remainder1 = cln::realpart(o.internalNumber());
			}
			cln::cl_R_div_t divt1, divt2;
			bool started = false, oneup = false, even = false;
			while(prec >= 0) {
				divt1 = cln::truncate2(remainder1);
				divt2 = cln::truncate2(remainder2);
				if(prec == 0) {
					if(oneup) {
						if(even) return divt1.quotient < 5 && divt2.quotient > 5;
						else return divt1.quotient <= 5 && divt2.quotient >= 5;
					} else {
						if(even) return (divt1.quotient > 5) == (divt2.quotient > 5);
						else return (divt1.quotient >= 5) == (divt2.quotient >= 5);
					}
				}
				if(started) {
					prec--;
					if(oneup && (divt1.quotient != 0 || divt2.quotient != 9)) return false;
					if(oneup) {
						even = false;
					} else {
						if(divt1.quotient != divt2.quotient) {
							if(divt1.quotient == divt2.quotient + 1) {
								oneup = true;							
							} else {
								return false;
							}
						}
						even = cln::evenp(divt2.quotient);
					}
				} else if(!cln::zerop(divt1.quotient)) {
					started = true;
					if(divt1.quotient < 10) {
						prec--;
						if(divt1.quotient != divt2.quotient) {
							if(divt2.quotient == 1) {
								oneup = true;
							} else {
								return false;
							}
						}
					} else {
						std::ostringstream s1;
						s1 << divt1.quotient;
						if(divt1.quotient == divt2.quotient) {
							prec -= s1.str().length();
						} else {
							std::ostringstream s2;
							s2 << divt2.quotient;
							string str1 = s1.str();
							string str2 = s2.str();						
							if(str1.length() != str2.length()) {
								if(str1.length() == str2.length() + 1 && str1[0] == '1') {
									oneup = true;
									prec--;
									str1.erase(str1.begin());
								} else {
									return false;
								}
							}
							for(size_t i = 0; i < str1.length(); i++) {							
								if(oneup && (str1[i] != '0' || str2[i] != '9')) return false;							
								if(oneup) {
									even = false;
								} else {
									if(str1[i] != str2[i]) {
										if(str1[i] == str2[i] + 1) {								
											oneup = true;
										} else {
											return false;
										}
									}
									even = str2[i] == '0' || str2[i] == '2' || str2[i] == '4' || str2[i] == '6' || str2[i] == '8';
								}
								prec--;
								if(prec == 0) {
									if(oneup) {
										if(even) return i + 1 < str1.length() && str1[i + 1] < '5' && str2[i + 1] > '5';
										else return i + 1 < str1.length() && str1[i + 1] <= '5' && str2[i + 1] >= '5';
									} else {
										if(even) return i == str1.length() - 1 || (str1[i + 1] > '5') == (str2[i + 1] > '5');
										else return i == str1.length() - 1 || (str1[i + 1] >= '5') == (str2[i + 1] >= '5');
									}
								}
							}
						}
					}
				}
				if(cln::zerop(remainder1) && cln::zerop(remainder2)) return !oneup;
				remainder1 = divt1.remainder * 10;
				remainder2 = divt2.remainder * 10;
			}
			return false;
		}
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}
	return false;
}
ComparisonResult Number::compare(const Number &o) const {
	if(b_inf || o.isInfinity()) return COMPARISON_RESULT_UNKNOWN;
	if(b_pinf) {
		if(o.isPlusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_LESS;
	}
	if(b_minf) {
		if(o.isMinusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) return COMPARISON_RESULT_GREATER;
	if(o.isMinusInfinity()) return COMPARISON_RESULT_LESS;
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(!isComplex() && !o.isComplex()) {
		int i = 0;
		if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
			i = mpfr_cmp_q(o.internalFloat(), r_value);
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isApproximateType()) i = -mpfr_cmp(f_value, o.internalFloat());
			else i = -mpfr_cmp_q(f_value, o.internalRational());
		} else {
			i = mpq_cmp(o.internalRational(), r_value);
		}
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i > 0) return COMPARISON_RESULT_GREATER;
		else return COMPARISON_RESULT_LESS;
	} else {
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareApproximately(const Number &o, int prec) const {
	if(b_inf || o.isInfinity()) return COMPARISON_RESULT_UNKNOWN;
	if(b_pinf) {
		if(o.isPlusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_LESS;
	}
	if(b_minf) {
		if(o.isMinusInfinity()) return COMPARISON_RESULT_EQUAL;
		else return COMPARISON_RESULT_GREATER;
	}
	if(o.isPlusInfinity()) return COMPARISON_RESULT_GREATER;
	if(o.isMinusInfinity()) return COMPARISON_RESULT_LESS;
	if(equalsApproximately(o, prec)) return COMPARISON_RESULT_EQUAL;
	if(!isComplex() && !o.isComplex()) {
		int i = 2;
		//cln::compare(cln::realpart(o.internalNumber()), cln::realpart(value));
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i == -1) return COMPARISON_RESULT_LESS;
		else if(i == 1) return COMPARISON_RESULT_GREATER;
		return COMPARISON_RESULT_UNKNOWN;
	} else {		
		return COMPARISON_RESULT_NOT_EQUAL;
	}
}
ComparisonResult Number::compareImaginaryParts(const Number &o) const {
	int i = cln::compare(cln::imagpart(o.internalNumber()), cln::imagpart(value));
	if(i == 0) return COMPARISON_RESULT_EQUAL;
	else if(i == -1) return COMPARISON_RESULT_LESS;
	else if(i == 1) return COMPARISON_RESULT_GREATER;
	return COMPARISON_RESULT_UNKNOWN;
}
ComparisonResult Number::compareRealParts(const Number &o) const {
	int i = cln::compare(cln::realpart(o.internalNumber()), cln::realpart(value));
	if(i == 0) return COMPARISON_RESULT_EQUAL;
	else if(i == -1) return COMPARISON_RESULT_LESS;
	else if(i == 1) return COMPARISON_RESULT_GREATER;
	return COMPARISON_RESULT_UNKNOWN;
}
bool Number::isGreaterThan(const Number &o) const {
	if(b_minf || b_inf || o.isInfinity() || o.isPlusInfinity()) return false;
	if(o.isMinusInfinity()) return true;
	if(b_pinf) return true;
	if(isComplex() || o.isComplex()) return false;
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalFloat(), r_value) < 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) return mpfr_greater_p(f_value, o.internalFloat());
		else return mpfr_cmp_q(f_value, o.internalRational()) > 0;
	}
	return mpq_cmp(r_value, o.internalRational()) > 0;
}
bool Number::isLessThan(const Number &o) const {
	if(o.isMinusInfinity() || o.isInfinity() || b_inf || b_pinf) return false;
	if(b_minf || o.isPlusInfinity()) return true;
	if(isComplex() || o.isComplex()) return false;
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		return mpfr_cmp_q(o.internalFloat(), r_value) > 0;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) return mpfr_less_p(f_value, o.internalFloat());
		else return mpfr_cmp_q(f_value, o.internalRational()) < 0;
	}
	return mpq_cmp(r_value, o.internalRational()) < 0;
}
bool Number::isGreaterThanOrEqualTo(const Number &o) const {
	if(b_inf || o.isInfinity()) return false;
	if(b_minf) return o.isMinusInfinity();
	if(b_pinf) return true;
	if(!isComplex() && !o.isComplex()) {
		if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalFloat(), r_value) <= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isApproximateType()) return mpfr_greaterequal_p(f_value, o.internalFloat());
			else return mpfr_cmp_q(f_value, o.internalRational()) >= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) >= 0;
	}
	return false;
}
bool Number::isLessThanOrEqualTo(const Number &o) const {
	if(b_inf || o.isInfinity()) return false;
	if(b_pinf) return o.isPlusInfinity();
	if(b_minf) return true;
	if(!isComplex() && !o.isComplex()) {
		if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
			return mpfr_cmp_q(o.internalFloat(), r_value) >= 0;
		} else if(n_type == NUMBER_TYPE_FLOAT) {
			if(o.isApproximateType()) return mpfr_lessequal_p(f_value, o.internalFloat());
			else return mpfr_cmp_q(f_value, o.internalRational()) <= 0;
		}
		return mpq_cmp(r_value, o.internalRational()) <= 0;
	}
	return false;
}
bool Number::isEven() const {
	return isInteger() && mpz_even_p(mpq_numref(r_value));
}
bool Number::denominatorIsEven() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && mpz_even_p(mpq_denref(r_value));
}
bool Number::denominatorIsTwo() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && mpz_cmp_si(mpq_denref(r_value), 2) == 0;
}
bool Number::numeratorIsEven() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && mpz_even_p(mpq_numref(r_value));
}
bool Number::numeratorIsOne() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && mpz_cmp_si(mpq_numref(r_value), 1) == 0;
}
bool Number::numeratorIsMinusOne() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && mpz_cmp_si(mpq_numref(r_value), -1) == 0;
}
bool Number::isOdd() const {
	return isInteger() && mpz_odd_p(mpq_numref(r_value));
}

int Number::integerLength() const {
	if(isInteger()) return cln::integer_length(cln::numerator(cln::rational(cln::realpart(value))));
	return 0;
}


bool Number::add(const Number &o) {
	if(b_inf) return !o.isInfinite();
	if(o.isInfinity()) {
		if(isInfinite()) return false;
		setInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(b_minf) return !o.isPlusInfinity();
	if(b_pinf) return !o.isMinusInfinity();
	if(o.isPlusInfinity()) {
		b_pinf = true;
		value = 0;
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		b_minf = true;
		value = 0;
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		mpfr_add_q(f_value, o.internalFloat(), r_value, DRM);
		n_type = NUMBER_TYPE_FLOAT;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) mpfr_add(f_value, f_value, o.internalFloat(), DRM);
		else mpfr_add_q(f_value, f_value, o.internalRational(), DRM);
	} else {
		mpq_add(r_value, r_value, o.internalRational());
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::subtract(const Number &o) {
	if(b_inf) {
		return !o.isInfinite();
	}
	if(o.isInfinity()) {
		if(isInfinite()) return false;
		setPrecisionAndApproximateFrom(o);
		setInfinity();
		return true;
	}
	if(b_pinf) {
		return !o.isPlusInfinity();
	}
	if(b_minf) {
		return !o.isMinusInfinity();
	}
	if(o.isPlusInfinity()) {
		setPlusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		setMinusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		mpfr_sub_q(f_value, o.internalFloat(), r_value, DRM);
		n_type = NUMBER_TYPE_FLOAT;
		mpfr_neg(f_value, f_value, DRM);
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) mpfr_sub(f_value, f_value, o.internalFloat(), DRM);
		else mpfr_sub_q(f_value, f_value, o.internalRational(), DRM);
	} else {
		mpq_sub(r_value, r_value, o.internalRational());
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::multiply(const Number &o) {
	if(o.isInfinite() && isZero()) return false;
	if(isInfinite() && o.isZero()) return false;
	if((isInfinite() && o.isComplex()) || (o.isInfinite() && isComplex())) {
		//setInfinity();
		//return true;
		return false;
	}
	if(isInfinity()) return true;
	if(o.isInfinity()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(b_pinf || b_minf) {
		if(o.isNegative()) {
			b_pinf = !b_pinf;
			b_minf = !b_minf;
			setPrecisionAndApproximateFrom(o);
		}
		return true;
	}
	if(o.isPlusInfinity()) {
		if(isNegative()) setMinusInfinity();
		else setPlusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isMinusInfinity()) {
		if(isNegative()) setPlusInfinity();
		else setMinusInfinity();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isZero()) return true;
	if(o.isZero()) {
		clear();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		mpfr_mul_q(f_value, o.internalFloat(), r_value, DRM);
		n_type = NUMBER_TYPE_FLOAT;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) mpfr_mul(f_value, f_value, o.internalFloat(), DRM);
		else mpfr_mul_q(f_value, f_value, o.internalRational(), DRM);
	} else {
		mpq_mul(r_value, r_value, o.internalRational());
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::divide(const Number &o) {
	if(isInfinite() && o.isInfinite()) return false;
	if(isInfinite() && o.isZero()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(o.isInfinite()) {
		clear();
		return true;
	}
	if(isInfinite()) {
		if(o.isComplex()) {
			//setInfinity();
			return false;
		} else if(o.isNegative()) {
			b_pinf = !b_pinf;
			b_minf = !b_minf;
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isZero()) {
		if(isZero()) return false;
		//division by zero!!!
		//setInfinity();
		//return true;
		return false;
	}
	if(isZero()) {
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isApproximateType() && n_type != NUMBER_TYPE_FLOAT) {
		mpq_inv(r_value, r_value);
		mpfr_div_q(f_value, o.internalFloat(), r_value, DRM);
		n_type = NUMBER_TYPE_FLOAT;
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		if(o.isApproximateType()) mpfr_div(f_value, f_value, o.internalFloat(), DRM);
		else mpfr_div_q(f_value, f_value, o.internalRational(), DRM);
	} else {
		mpq_div(r_value, r_value, o.internalRational());
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::recip() {
	if(isZero()) {
		//division by zero!!!
		//setInfinity();
		//return true;
		return false;
	}
	if(isInfinite()) {
		clear();
		return true;
	}
	if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_ui_div(f_value, 1, f_value, DRM);
	} else {
		mpq_inv(r_value, r_value);
	}
	removeFloatZeroPart();
	return true;
}
bool Number::raise(const Number &o, bool try_exact) {
	if(o.isInfinity()) return false;
	if(isInfinite()) {	
		if(o.isNegative()) {
			clear();
			return true;
		}
		if(o.isZero()) {
			return false;
		}
		if(isMinusInfinity()) {
			if(o.isEven()) {
				setPlusInfinity();
			} else if(!o.isInteger()) {
				//setInfinity();
				return false;
			}
		}
		return true;
	}
	if(o.isMinusInfinity()) {
		if(isZero()) {
			//setInfinity();
			return false;
		} else if(isComplex()) {
			return false;
		} else {
			clear();
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isPlusInfinity()) {
		if(isZero()) {
		} else if(isComplex() || isNegative()) {
			return false;
		} else {
			setPlusInfinity();
		}
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(isZero() && o.isNegative()) {
		CALCULATOR->error(true, _("Division by zero."), NULL);
		return false;
	}
	if(isZero()) {
		if(o.isZero()) {
			//0^0
			CALCULATOR->error(false, _("0^0 might be considered undefined"), NULL);
			set(1, 1);
			setApproximate(o.isApproximate());
			return true;
		} else if(o.isComplex()) {
			CALCULATOR->error(false, _("The result of 0^i is possibly undefined"), NULL);
		} else {
			return true;
		}
	}
	if(try_exact && n_type == NUMBER_TYPE_RATIONAL && !o.isApproximateType()) {
		if(mpz_fits_sint_p(mpq_numref(o.internalRational())) != 0 && mpz_fits_uint_p(mpq_denref(o.internalRational())) != 0) {
			int i_pow = mpz_get_si(mpq_numref(o.internalRational()));
			unsigned int i_root = mpz_get_ui(mpq_denref(o.internalRational()));
			if(i_pow < 0) {
				mpq_inv(r_value, r_value);
				i_pow = -i_pow;
			}
			if(isInteger()) {
				if(i_pow != 1) {
					mpz_pow_ui(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) i_pow);
				}
				if(i_root != 1) {
					mpq_t r_test;
					mpq_init(r_test);
					mpq_set(r_test, r_value);
					if(mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) != 0) {
						mpq_set(r_value, r_test);
						mpq_clear(r_test);
						return true;
					}
					mpq_clear(r_test);
				} else {
					return true;
				}
			} else {
				if(i_pow != 1) {
					mpz_pow_ui(mpq_numref(r_value), mpq_numref(r_value), (unsigned long int) i_pow);
					mpz_pow_ui(mpq_denref(r_value), mpq_denref(r_value), (unsigned long int) i_pow);
					mpq_canonicalize(r_value);
				}
				if(i_root != 1) {
					mpq_t r_test;
					mpq_init(r_test);
					mpq_set(r_test, r_value);
					if(mpz_root(mpq_numref(r_test), mpq_numref(r_test), i_root) != 0) {
						if(mpz_root(mpq_denref(r_test), mpq_denref(r_test), i_root) != 0) {
							mpq_set(r_value, r_test);
							mpq_canonicalize(r_value);
							mpq_clear(r_test);
							return true;
						}
					}
					mpq_clear(r_test);
				} else {
					return true;
				}
			}
		}
	}
	if(n_type != NUMBER_TYPE_FLOAT) {
		mpfr_set_q(f_value, r_value, DRM);
		n_type = NUMBER_TYPE_FLOAT;
	}
	if(o.isApproximateType()) {
		mpfr_pow(f_value, f_value, o.internalFloat(), DRM);
	} else if(o.isInteger()) {
		mpfr_pow_z(f_value, f_value, mpq_numref(o.internalRational()), DRM);
	} else {
		mpfr_t f_pow;
		mpfr_init2(f_pow, BIT_PRECISION);
		mpfr_set_q(f_pow, o.internalRational(), DRM);
		mpfr_pow(f_value, f_value, f_pow, DRM);
		mpfr_clear(f_pow);
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	testApproximate();
	testInteger();
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
		setPlusInfinity();
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_mul(r_value, r_value, r_value);
	} else {
		mpfr_sqr(f_value, f_value, DRM);
	}
	return true;
}

bool Number::negate() {
	if(isInfinite()) {
		b_pinf = !b_pinf;
		b_minf = !b_minf;
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_neg(r_value, r_value);
	} else {
		mpfr_neg(f_value, f_value, DRM);
	}
	return true;
}
void Number::setNegative(bool is_negative) {
	if(!isZero() && minusp(cln::realpart(value)) != is_negative) {
		if(isInfinite()) {b_pinf = !b_pinf; b_minf = !b_minf; return;}
		try {
			value = cln::complex(-cln::realpart(value), cln::imagpart(value));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		}
	}
}
bool Number::abs() {
	if(isInfinite()) {
		setPlusInfinity();
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) {
		mpq_abs(r_value, r_value);
	} else {
		mpfr_abs(f_value, f_value, DRM);
	}
	return true;
}
bool Number::signum() {
	if(isInfinite()) return false;
	value = cln::signum(value);
	return true;
}
bool Number::round() {
	if(isInfinite() || isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_t i_rem;
			mpz_init(i_rem);
			mpz_mul_ui(mpq_numref(r_value), mpq_numref(r_value), 2);
			mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_mul_ui(mpq_denref(r_value), mpq_denref(r_value), 2);
			mpz_fdiv_qr(mpq_numref(r_value), i_rem, mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
			if(mpz_sgn(i_rem) == 0 && mpz_odd_p(mpq_numref(r_value))) {
				if(mpz_sgn(mpq_numref(r_value)) < 0) mpz_add(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
				else mpz_sub(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			}
			mpz_clear(i_rem);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), f_value, MPFR_RNDN);
		n_type = NUMBER_TYPE_RATIONAL;
	}
	return true;
}
bool Number::floor() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_fdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), f_value, MPFR_RNDD);
		n_type = NUMBER_TYPE_RATIONAL;
	}
	return true;
}
bool Number::ceil() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_cdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), f_value, MPFR_RNDU);
		n_type = NUMBER_TYPE_RATIONAL;
	}
	return true;
}
bool Number::trunc() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(!isInteger()) {
			mpz_tdiv_q(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
			mpz_set_ui(mpq_denref(r_value), 1);
		}
	} else {
		mpz_set_ui(mpq_denref(r_value), 1);
		mpfr_get_z(mpq_numref(r_value), f_value, MPFR_RNDZ);
		n_type = NUMBER_TYPE_RATIONAL;
	}
	return true;
}
bool Number::round(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && round();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::round1(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::floor(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && floor();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::floor1(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::ceil(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && ceil();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::ceiling1(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::trunc(const Number &o) {
	if(isInfinite() || o.isInfinite()) {
		return divide(o) && trunc();
	}
	if(isComplex()) return false;
	if(o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::truncate1(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::mod(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::mod(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::frac() {
	if(isInfinite() || isComplex()) return false;
	if(n_type == NUMBER_TYPE_RATIONAL) {
		if(isInteger()) {
			clear();
		} else {
			mpz_tdiv_r(mpq_numref(r_value), mpq_numref(r_value), mpq_denref(r_value));
		}
	} else {
		mpfr_frac(f_value, f_value, DRM); 
	}
	return true;
}
bool Number::rem(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	
	cln::cl_N new_value;
	try {
		new_value = cln::rem(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::smod(const Number &o) {
	if(!isInteger() || !o.isInteger()) return false;
	cln::cl_N new_value;
	try {
		const cln::cl_I b2 = cln::ceiling1(cln::numerator(cln::rational(cln::realpart(o.internalNumber()))) >> 1) - 1;
		new_value = cln::mod(cln::numerator(cln::rational(cln::realpart(value))) + b2, cln::numerator(cln::rational(cln::realpart(o.internalNumber())))) - b2;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}	
bool Number::irem(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::rem(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	return true;
}
bool Number::irem(const Number &o, Number &q) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
/*	try {
		const cln::cl_I_div_t rem_quo = cln::truncate2(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		q.setInternal(rem_quo.quotient);
		value = rem_quo.remainder;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}*/
	return true;
}
bool Number::iquo(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
/*	cln::cl_N new_value;
	try {
		new_value = cln::truncate1(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;*/
	return true;
}
bool Number::iquo(const Number &o, Number &r) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
/*	try {
		const cln::cl_I_div_t rem_quo = cln::truncate2(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		r.setInternal(rem_quo.remainder);
		value = rem_quo.quotient;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
	}*/
	return true;
}
bool Number::isqrt() {
/*	if(isInteger()) {
		cln::cl_N new_value;
		try {
			cln::cl_I iroot;
			cln::isqrt(cln::numerator(cln::rational(cln::realpart(value))), &iroot);
			new_value = iroot;
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
			return false;
		}
		value = new_value;
		return true;
	}*/
	return false;
}

int Number::getBoolean() const {
	if(isPositive()) {
		return 1;
	} else if(isNonPositive()) {
		return 0;
	}
	return -1;
}
void Number::toBoolean() {
	setTrue(isPositive());
}
void Number::setTrue(bool is_true) {
	if(is_true) {
		value = 1;
	} else {
		value = 0;
	}
}
void Number::setFalse() {
	setTrue(false);
}
void Number::setLogicalNot() {
	setTrue(!isPositive());
}

void Number::e() {
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_set_ui(f_value, 1, DRM);
	mpfr_exp(f_value, f_value, DRM);
}
void Number::pi() {
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_const_pi(f_value, DRM);
}
void Number::catalan() {
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_const_catalan(f_value, DRM);
}
void Number::euler() {
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_const_euler(f_value, DRM);
}
bool Number::zeta() {
	if(isOne()) {
		setInfinity();
		return true;
	}
	if(isNegative() || !isInteger() || isZero()) {
		CALCULATOR->error(true, _("Can only handle Riemann Zeta with an integer argument (s) >= 1"), NULL);
		return false;
	}
	
	bool overflow = false;
	long int i = intValue(&overflow);
	if(overflow) {
		CALCULATOR->error(true, _("Cannot handle an argument (s) that large for Riemann Zeta."), NULL);
		return false;
	}
	mpfr_zeta_ui(f_value, (unsigned int) i, DRM);
	n_type = NUMBER_TYPE_FLOAT;
	return true;
}

bool Number::sin() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::sin(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::asin() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::asin(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::sinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::sinh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::asinh() {
	if(isInfinite()) return true;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::asinh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::cos() {
	if(isInfinite()) return false;
	if(isZero() && !isApproximate()) {
		set(1);
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::cos(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}	
bool Number::acos() {
	if(isInfinite()) return false;
	if(isOne() && !isApproximate()) {
		clear();
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::acos(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::cosh() {
	if(isInfinite()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(isZero() && !isApproximate()) {
		set(1);
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::cosh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::acosh() {
	if(isPlusInfinity() || isInfinity()) return true;
	if(isMinusInfinity()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::acosh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::tan() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::tan(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::atan() {
	if(isInfinity()) return false;
	if(isZero()) return true;
	if(isInfinite()) {
		pi();
		divide(2);
		if(isMinusInfinity()) negate();
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::atan(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::tanh() {
	if(isInfinity()) return true;
	if(isPlusInfinity()) set(1);
	if(isMinusInfinity()) set(-1);
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::tanh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::atanh() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	if(isOne()) {
		setPlusInfinity();
		return true;
	}
	if(isMinusOne()) {
		setMinusInfinity();
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::atanh(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	value = new_value;
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::ln() {
	if(isPlusInfinity()) return true;
	if(isInfinite()) return false;
	if(isOne() && !isApproximate()) {
		clear();
		return true;
	}
	if(isZero()) {
		setMinusInfinity();
		return true;
	}
	
	if(n_type == NUMBER_TYPE_RATIONAL) mpfr_set_q(f_value, r_value, DRM);
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_log(f_value, f_value, DRM);
	removeFloatZeroPart();
	testApproximate();
	testInteger();
	return true;
}
bool Number::log(const Number &o) {
	if(isPlusInfinity()) return true;
	if(isInfinite()) return false;
	if(isOne()) {
		bool was_approx = b_approx || o.isApproximate();
		clear();
		b_approx = was_approx;
		return true;
	}
	if(isZero()) {
		bool was_approx = b_approx || o.isApproximate();
		setMinusInfinity();
		b_approx = was_approx;
		return true;
	}
	if(o.isZero()) {
		clear();
		setPrecisionAndApproximateFrom(o);
		return true;
	}
	if(o.isOne()) {
		//setInfinity();
		//return true;
		return false;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) mpfr_set_q(f_value, r_value, DRM);
	n_type = NUMBER_TYPE_FLOAT;
	if(o.isRational()) {
		if(o.isTwo()) {
			mpfr_log2(f_value, f_value, DRM);
		} else if(o.isTen()) {
			mpfr_log10(f_value, f_value, DRM);
		} else {
			mpfr_t f_base;
			mpfr_init2(f_base, BIT_PRECISION);
			mpfr_set_q(f_base, o.internalRational(), DRM);
			mpfr_log(f_value, f_value, DRM);
			mpfr_log(f_base, f_base, DRM);
			mpfr_div(f_value, f_value, f_base, DRM);
			mpfr_clear(f_base);
		}
	} else {
		mpfr_t f_base;
		mpfr_init2(f_base, BIT_PRECISION);
		mpfr_set(f_base, o.internalFloat(), DRM);
		mpfr_log(f_value, f_value, DRM);
		mpfr_log(f_base, f_base, DRM);
		mpfr_div(f_value, f_value, f_base, DRM);
		mpfr_clear(f_base);
	}
	removeFloatZeroPart();
	setPrecisionAndApproximateFrom(o);
	testApproximate();
	testInteger();
	return true;
}
bool Number::exp() {
	if(isInfinity()) return false;
	if(isPlusInfinity()) return true;
	if(isMinusInfinity()) {
		clear();
		return true;
	}
	if(n_type == NUMBER_TYPE_RATIONAL) mpfr_set_q(f_value, r_value, DRM);
	n_type = NUMBER_TYPE_FLOAT;
	mpfr_exp(f_value, f_value, DRM);
	testApproximate();
	testInteger();
	return true;
}
bool Number::lambertW() {
	if(!isReal()) return false;
	if(isZero()) return true;
	cln::cl_R x = cln::realpart(value);
	cln::cl_R m1_div_exp1 = -1 / cln::exp1();
	if(x == m1_div_exp1) {
		value = -1;
		if(!b_approx) {
			i_precision = PRECISION;
			b_approx = true;
		}
		return true;
	}
	if(x < m1_div_exp1) return false;	
	cln::cl_R w = 0;
	if(x > 10) {
		try {
			w = cln::ln(x) - cln::ln(cln::ln(x));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
			return false;
		}
	}
	cln::cl_RA wPrec;
	try {
		wPrec = cln::expt(cln::cl_I(10), -(PRECISION + 2));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
		return false;
	}
	cln::cl_N new_value = value;
	while(true) {
		try {
			cln::cl_R wTimesExpW = w * cln::exp(w);
			cln::cl_R wPlusOneTimesExpW = (w + 1) * cln::exp(w);
			if(wPrec > cln::abs((x - wTimesExpW) / wPlusOneTimesExpW)) {
				new_value = w;
				break;
			}
			w = w - (wTimesExpW - x) / (wPlusOneTimesExpW - (w + 2) * (wTimesExpW - x) / (2 * w + 2));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
			return false;
		}
	}
	value = new_value;
	if(!b_approx) {
		i_precision = PRECISION;
		b_approx = true;
	}
	testInteger();
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
		setPlusInfinity();
		return true;
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
	if(k.isNegative()) return false;
	if(m.isZero() || m.isNegative()) return false;
	if(k.isGreaterThan(m)) return false;
	if(k.isZero()) {
		set(1);
	} else if(k.isOne()) {
		set(m);
		setPrecisionAndApproximateFrom(k);
	} else if(m.equals(k)) {
		set(1);
		setPrecisionAndApproximateFrom(m);
		setPrecisionAndApproximateFrom(k);
	} else {		
		cln::cl_N new_value;
		cl_I im;
		cl_I ik;
		try {
			ik = cln::numerator(cln::rational(cln::realpart(k.internalNumber())));
			im = cln::numerator(cln::rational(cln::realpart(m.internalNumber())));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
			return false;
		}
		if(im > long(INT_MAX) || ik > long(INT_MAX)) {
			ik = cln::minus1(ik);
			Number k_fac(k);
			try {
				k_fac.factorial();
				cl_I ithis = im;
				for(; !cln::zerop(ik); ik = cln::minus1(ik)) {
					im = cln::minus1(im);
					ithis = ithis * im;
				}
				new_value = ithis;
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
				return false;
			}
			clear();
			value = new_value;
			divide(k_fac);
		} else {
			try {
				new_value = cln::binomial(cl_I_to_uint(im), cl_I_to_uint(ik));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what());
				return false;
			}
			clear();
			value = new_value;
		}		
		setPrecisionAndApproximateFrom(m);
		setPrecisionAndApproximateFrom(k);
	}
	return true;
}

bool Number::factorize(vector<Number> &factors) {
	if(isZero() || !isInteger()) return false;
	if(mpz_cmp_si(mpq_numref(r_value), 1) == 0) {
		factors.push_back(Number(1, 1));
		return true;
	}
	if(mpz_cmp_si(mpq_numref(r_value), -1) == 0) {
		factors.push_back(Number(-1, 1));
		return true;
	}
	mpz_t inr, last_prime, facmax;
	mpz_inits(inr, last_prime, facmax, NULL);
	mpz_set(inr, mpq_numref(r_value));
	if(mpz_sgn(inr) < 0) {
		mpz_neg(inr, inr);
		factors.push_back(Number(-1, 1));
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
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 == COMPARISON_RESULT_UNKNOWN && (i2 == COMPARISON_RESULT_UNKNOWN || i2 != COMPARISON_RESULT_LESS)) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN && (i1 != COMPARISON_RESULT_LESS)) return false;
			setTrue(i1 == COMPARISON_RESULT_LESS || i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_LOGICAL_XOR: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) return false;
			if(i1 == COMPARISON_RESULT_LESS) setTrue(i2 != COMPARISON_RESULT_LESS);
			else setTrue(i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_LOGICAL_AND: {
			Number nr;
			ComparisonResult i1 = compare(nr);
			ComparisonResult i2 = o.compare(nr);
			if(i1 == COMPARISON_RESULT_UNKNOWN || i1 == COMPARISON_RESULT_EQUAL_OR_LESS || i1 == COMPARISON_RESULT_NOT_EQUAL) i1 = COMPARISON_RESULT_UNKNOWN;
			if(i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_EQUAL_OR_LESS || i2 == COMPARISON_RESULT_NOT_EQUAL) i2 = COMPARISON_RESULT_UNKNOWN;
			if(i1 == COMPARISON_RESULT_UNKNOWN && (i2 == COMPARISON_RESULT_UNKNOWN || i2 == COMPARISON_RESULT_LESS)) return false;
			if(i2 == COMPARISON_RESULT_UNKNOWN && (i1 == COMPARISON_RESULT_LESS)) return false;
			setTrue(i1 == COMPARISON_RESULT_LESS && i2 == COMPARISON_RESULT_LESS);
			return true;
		}
		case OPERATION_EQUALS: {
			ComparisonResult i = compare(o);
			if(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
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
			if(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL_OR_LESS) return false;
			setTrue(i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_GREATER || i == COMPARISON_RESULT_LESS);
			return true;
		}
	}
	return false;	
}
string Number::printNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_numref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_denref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_numref(r_value), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printMPZ(mpq_denref(r_value), base, display_sign, base_display, lower_case);
}

string Number::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
	/*if(isApproximateType() && !isInfinite() && !isComplex()) {
		if((PRECISION < cln::float_format_lfloat_min && cln::zerop(cln::truncate2(MIN_PRECISION_FLOAT_RE(value)).remainder))
		|| (PRECISION >= cln::float_format_lfloat_min && cln::zerop(cln::truncate2(REAL_PRECISION_FLOAT_RE(value)).remainder))) {
			Number nr_copy(*this);
			nr_copy.testInteger();
			return nr_copy.print(po, ips);
		}
	}*/
	if(ips.minus) *ips.minus = false;
	if(ips.exp_minus) *ips.exp_minus = false;
	if(ips.num) *ips.num = "";
	if(ips.den) *ips.den = "";
	if(ips.exp) *ips.exp = "";
	if(ips.re) *ips.re = "";
	if(ips.im) *ips.im = "";
	if(po.is_approximate && isApproximate()) *po.is_approximate = true;
	if((po.base == BASE_SEXAGESIMAL || po.base == BASE_TIME) && isReal()) {
		Number nr(*this);
		bool neg = nr.isNegative();
		nr.setNegative(false);
		nr.trunc();
		string str = nr.printNumerator(10, false);
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
			str += "'";
		}	
		nr2.frac();
		if(!nr2.isZero() || po.base == BASE_SEXAGESIMAL) {
			nr2.multiply(60);
			nr = nr2;
			nr.trunc();
			nr2.frac();
			if(!nr2.isZero()) {
				if(po.is_approximate) *po.is_approximate = true;
				if(nr2.isGreaterThanOrEqualTo(Number(1, 2))) {
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
				str += "\"";
			}
		}
		if(ips.minus) {
			*ips.minus = neg;
		} else if(neg) {
			str.insert(0, "-");
		}
		if(ips.num) *ips.num = str;
		
		return str;
	}
	string str;
	int base;
	int min_decimals = 0;
	if(po.use_min_decimals && po.min_decimals > 0) min_decimals = po.min_decimals;
	if((int) min_decimals > po.max_decimals && po.use_max_decimals && po.max_decimals >= 0) {
		min_decimals = po.max_decimals;
	}
	if(po.base <= 1 && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_TIME) base = 10;
	else if(po.base > 36 && po.base != BASE_SEXAGESIMAL) base = 36;
	else base = po.base;
	if(isApproximateType() && po.base == BASE_ROMAN_NUMERALS) base = 10;
	long int precision = PRECISION;
	if(b_approx && i_precision > 0 && i_precision < PRECISION) precision = i_precision;
	if(po.restrict_to_parent_precision && ips.parent_precision > 0 && ips.parent_precision < precision) precision = ips.parent_precision;
	if(isComplex()) {
		/*bool bre = hasRealPart();
		if(bre) {
			Number re, im;
			re.setInternal(cln::realpart(value));
			im.setInternal(cln::imagpart(value));
			if(isApproximate()) {
				re.setApproximate();
				im.setApproximate();
			}
			str = re.print(po, ips);
			if(ips.re) *ips.re = str;
			InternalPrintStruct ips_n = ips;
			bool neg = false;
			ips_n.minus = &neg;
			string str2 = im.print(po, ips_n);
			if(ips.im) *ips.im = str2;
			if(*ips_n.minus) {
				str += " - ";
			} else {
				str += " + ";
			}
			str += str2;	
		} else {
			Number im;
			im.setInternal(cln::imagpart(value));
			if(isApproximate()) {
				im.setApproximate();
			}
			str = im.print(po, ips);
			if(ips.im) *ips.im = str;
		}
		if(!po.short_multiplication) {
			if(po.spacious) {
				str += " * ";
			} else {
				str += "*";
			}
		}
		str += "i";
		if(po.is_approximate && isApproximate()) *po.is_approximate = true;
		if(ips.num) *ips.num = str;*/
	} else if(isInteger()) {
		mpz_t ivalue;
		mpz_init_set(ivalue, mpq_numref(r_value));
		bool neg = (mpz_sgn(ivalue) < 0);
		bool rerun = false;
		bool exact = true;

		integer_rerun:

		string mpz_str = printMPZ(ivalue, base, false, BASE_DISPLAY_NONE, po.lower_case_numbers);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		
		long int expo = 0;
		if(base == 10) {
			if(mpz_str.length() > 0 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				expo = mpz_str.length() - 1;
			} else if(mpz_str.length() > 0) {
				for(int i = mpz_str.length() - 1; i >= 0; i--) {
					if(mpz_str[i] != '0') {
						break;
					}
					expo++;
				} 
			}
			if(po.min_exp == EXP_PRECISION) {
				if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
					expo = 0;
				}
			} else if(po.min_exp == EXP_BASE_3) {
				expo -= expo % 3;
			} else if(po.min_exp != 0) {
				if((long int) expo > -po.min_exp && (long int) expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		
		bool dp_added = false;

		if(!rerun && mpz_sgn(ivalue) != 0) {
			long int precision2 = precision;
			if(base != 10) {
				Number precmax(10);
				precmax.raise(precision);
				precmax--;
				precmax.log(base);
				precmax.floor();
				precision2 = precmax.intValue();
			}
			precision2 -= mpz_str.length();
			if(po.use_max_decimals && po.max_decimals >= 0 && po.max_decimals < expo && po.max_decimals - expo < precision2 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
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
					mpz_clears(i_rem, i_quo, i_div, i_div, NULL);
					goto integer_rerun;
				}
				mpz_clears(i_rem, i_quo, i_div, NULL);
			} else if(precision2 < 0 && ((expo > 0 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) || isApproximate() || (ips.parent_approximate && po.restrict_to_parent_precision))) {
				precision2 = -precision2;

				mpq_t qvalue;
				mpq_init(qvalue);
				mpz_set(mpq_numref(qvalue), ivalue);

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
		int decimals = 0;
		if(expo > 0) {
			if(po.number_fraction_format == FRACTION_DECIMAL) {
				mpz_str.insert(mpz_str.length() - expo, po.decimalpoint());
				dp_added = true;
				decimals = expo;
			} else if(po.number_fraction_format == FRACTION_DECIMAL_EXACT) {
				mpz_str.insert(mpz_str.length() - expo, po.decimalpoint());
				dp_added = true;
				decimals = expo;
			} else {
				mpz_str = mpz_str.substr(0, mpz_str.length() - expo);
			}
		}
		if(ips.minus) *ips.minus = neg;
		str = format_number_string(mpz_str, base, po.base_display, !ips.minus && neg);
		if(base != BASE_ROMAN_NUMERALS && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
			int pos = str.length() - 1;
			for(; pos >= (int) str.length() + min_decimals - decimals; pos--) {
				if(str[pos] != '0') {
					break;
				}
			}
			if(pos + 1 < (int) str.length()) str = str.substr(0, pos + 1);
			if(exact && min_decimals > decimals) {
				if(decimals <= 0) {
					str += po.decimalpoint();
					dp_added = true;
				}
				while(min_decimals > decimals) {
					decimals++;
					str += "0";
				}
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				dp_added = false;
			}
		}
		if(!exact && po.is_approximate) *po.is_approximate = true;
		if(po.show_ending_zeroes && (!exact || isApproximate() || (ips.parent_approximate && po.restrict_to_parent_precision)) && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals)) {
			if(base != 10) {
				Number precmax(10);
				precmax.raise(precision);
				precmax--;
				precmax.log(base);
				precmax.floor();
				precision = precmax.intValue();
			}
			precision -= str.length();
			if(dp_added) {
				precision += 1;
			} else if(precision > 0) {
				str += po.decimalpoint();
			}
			for(; precision > 0 && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals); precision--) {
				decimals++;
				str += "0";
			}
		}
		if(expo != 0) { 
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
	} else if(isInfinity()) {
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str = SIGN_INFINITY;
		} else {
			str = _("infinity");
		}
	} else if(isPlusInfinity()) {
		str = "(";
		str += "+";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
		str += ")";
	} else if(isMinusInfinity()) {
		str = "(";
		str += "-";
		if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_INFINITY, po.can_display_unicode_string_arg))) {
			str += SIGN_INFINITY;
		} else {
			str += _("infinity");
		}
		str += ")";
	} else if(n_type == NUMBER_TYPE_FLOAT) {
		mpfr_t v;
		mpfr_init2(v, BIT_PRECISION);
		mpfr_set(v, f_value, DRM);
		bool neg = (mpfr_sgn(v) < 0);
		if(base != 10) {
			Number precmax(10);
			precmax.raise(precision);
			precmax--;
			precmax.log(base);
			precmax.floor();
			precision = precmax.intValue();
		}
		if(neg) mpfr_neg(v, v, DRM);
		mpfr_t f_log, f_base, f_log_base;
		mpfr_inits2(BIT_PRECISION, f_log, f_base, f_log_base, NULL);
		mpfr_set_si(f_base, base, DRM);
		mpfr_log(f_log, v, DRM);
		mpfr_log(f_log_base, f_base, DRM);
		mpfr_div(f_log, f_log, f_log_base, DRM);
		mpfr_trunc(f_log, f_log);
		mpfr_sub_si(f_log, f_log, precision - 1, DRM);
		long int l10 = -mpfr_get_si(f_log, DRM);
		mpfr_pow(f_log, f_base, f_log, DRM);
		mpfr_div(v, v, f_log, DRM);
		if(po.round_halfway_to_even) mpfr_rint(v, v, MPFR_RNDN);
		else mpfr_round(v, v);
		mpz_t ivalue;
		mpz_init(ivalue);
		mpfr_get_z(ivalue, v, DRM);
		str = printMPZ(ivalue, base, true, BASE_DISPLAY_NONE, po.lower_case_numbers);
		long int expo = 0;
		if(base == 10) {
			expo = str.length() - l10 - 1;
			if(po.min_exp == EXP_PRECISION) {
				if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
					expo = 0;
				}
			} else if(po.min_exp == EXP_BASE_3) {
				expo -= expo % 3;
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		if(expo != 0) {
			l10 += expo;
		}
		bool has_decimal = false;
		if(l10 > 0) {
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}				
			str.insert(l10, po.decimalpoint());
			has_decimal = true;
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			if(l2 > 0 && !po.show_ending_zeroes) {
				if(min_decimals > 0) {
					int decimals = str.length() - l10 - 1;
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				has_decimal = false;
			}
		} else if(l10 < 0) {
			while(l10 < 0) {
				l10++;
				str += "0";
			}
		}

		if(str.empty()) {
			str = "0";
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
			has_decimal = false;
		}
		
		str = format_number_string(str, base, po.base_display, !ips.minus && neg, !has_decimal);
		
		if(expo != 0) {
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
		mpfr_clears(v, f_log, f_base, f_log_base, NULL);
	} else if(base != BASE_ROMAN_NUMERALS && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
		mpz_t num, d, remainder, remainder2, exp;
		mpz_inits(num, d, remainder, remainder2, exp, NULL);
		mpz_set(d, mpq_denref(r_value));
		mpz_set(num, mpq_numref(r_value));
		bool neg = (mpz_sgn(num) < 0);
		if(neg) mpz_neg(num, num);
		int l10 = 0;
		mpz_tdiv_qr(num, remainder, num, d);
		bool exact = (mpz_sgn(remainder) == 0);
		vector<mpz_t*> remainders;
		bool infinite_series = false;
		if(base != 10) {
			Number precmax(10);
			precmax.raise(precision);
			precmax--;
			precmax.log(base);
			precmax.floor();
			precision = precmax.intValue();
		}
		bool started = false;
		long int expo = 0;
		long int precision2 = precision;
		if(mpz_sgn(num) != 0) {
			str = printMPZ(num, base, true, BASE_DISPLAY_NONE);
			if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
			if(base != 10) {
				expo = 0;
			} else {
				expo = str.length() - 1;
				if(po.min_exp == EXP_PRECISION) {
					if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
						expo = 0;
					}
				} else if(po.min_exp == EXP_BASE_3) {
					expo -= expo % 3;
				} else if(po.min_exp != 0) {
					if(expo > -po.min_exp && expo < po.min_exp) { 
						expo = 0;
					}
				} else {
					expo = 0;
				}
			}
			precision2 -= str.length();
			int do_div = 0;;
			if(po.use_max_decimals && po.max_decimals >= 0 && po.max_decimals < expo && po.max_decimals - expo < precision2) {
				do_div = 1;
				
			} else if(precision2 < 0) {
				do_div = 2;
			}
			if(do_div) {
				mpz_t i_rem, i_quo, i_div, i_div_pre;
				mpz_inits(i_rem, i_quo, i_div, i_div_pre, NULL);
				mpz_ui_pow_ui(i_div_pre, (unsigned long int) base, do_div == 1 ? (unsigned long int) -(po.max_decimals - expo) : (unsigned long int) -precision2);
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
		}
		if(!exact && po.use_max_decimals && po.max_decimals >= 0 && precision2 > po.max_decimals - expo) precision2 = po.max_decimals - expo;
		bool try_infinite_series = po.indicate_infinite_series && !isApproximateType();
		while(!exact && precision2 > 0) {
			if(try_infinite_series) {
				mpz_t *remcopy = new mpz_t[1];
				mpz_init_set(*remcopy, remainder);
				remainders.push_back(remcopy);
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
			if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
			l10++;
			mpz_set(remainder, remainder2);
			if(try_infinite_series && !exact) {
				for(size_t i = 0; i < remainders.size(); i++) {
					if(CALCULATOR->aborted()) {mpz_clears(num, d, remainder, remainder2, exp, NULL); return CALCULATOR->abortedMessage();}
					if(!mpz_cmp(*remainders[i], remainder)) {
						infinite_series = true;
						try_infinite_series = false;
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
		if(!exact && !infinite_series) {
			if(po.number_fraction_format == FRACTION_DECIMAL_EXACT && !isApproximate()) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				mpz_clears(num, d, remainder, remainder2, exp, NULL);
				return print(po2, ips);
			}
			if(po.is_approximate) *po.is_approximate = true;
		}
		str = printMPZ(num, base, true, BASE_DISPLAY_NONE, po.lower_case_numbers);
		mpz_clears(num, d, remainder, remainder2, exp, NULL);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		if(base == 10) {
			expo = str.length() - l10 - 1;
			if(po.min_exp == EXP_PRECISION) {
				if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
					expo = 0;
				}
			} else if(po.min_exp == EXP_BASE_3) {
				expo -= expo % 3;
			} else if(po.min_exp != 0) {
				if(expo > -po.min_exp && expo < po.min_exp) { 
					expo = 0;
				}
			} else {
				expo = 0;
			}
		}
		if(expo != 0) {
			l10 += expo;
		}
		bool has_decimal = false;
		if(l10 > 0) {
			l10 = str.length() - l10;
			if(l10 < 1) {
				str.insert(str.begin(), 1 - l10, '0');
				l10 = 1;
			}				
			str.insert(l10, po.decimalpoint());
			has_decimal = true;
			int l2 = 0;
			while(str[str.length() - 1 - l2] == '0') {
				l2++;
			}
			if(l2 > 0 && !infinite_series && (exact || !po.show_ending_zeroes)) {
				if(min_decimals > 0) {
					int decimals = str.length() - l10 - 1;
					if(decimals - min_decimals < l2) l2 = decimals - min_decimals;
				}
				if(l2 > 0) str = str.substr(0, str.length() - l2);
			}
			if(str[str.length() - 1] == po.decimalpoint()[0]) {
				str.erase(str.end() - 1);
				has_decimal = false;
			}
		}
		int decimals = 0;
		if(l10 > 0) {
			decimals = str.length() - l10 - 1;
		}

		if(str.empty()) {
			str = "0";
		}
		if(exact && min_decimals > decimals) {
			if(decimals <= 0) {
				str += po.decimalpoint();
				decimals = 0;
				has_decimal = true;
			}
			for(; decimals < min_decimals; decimals++) {
				str += "0";
			}
		}
		if(str[str.length() - 1] == po.decimalpoint()[0]) {
			str.erase(str.end() - 1);
			has_decimal = false;
		}
		
		str = format_number_string(str, base, po.base_display, !ips.minus && neg, !has_decimal);
		
		if(infinite_series) {
			str += "...";
		}
		if(expo != 0) {
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
		Number num, den;
		num.setInternal(mpq_numref(r_value));
		den.setInternal(mpq_denref(r_value));
		if(isApproximate()) {
			num.setApproximate();
			den.setApproximate();
		}
		str = num.print(po, ips);
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
		string str2 = den.print(po, ips_n);
		if(ips.den) *ips.den = str2;
		str += str2;
		if(po.is_approximate && isApproximate()) *po.is_approximate = true;
	}
	return str;
}


