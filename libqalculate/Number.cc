/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2017  Hanna Knutsson (hanna.knutsson@protonmail.com)

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

#define REAL_PRECISION_FLOAT_RE(x)		cln::cl_float(cln::realpart(x), cln::float_format(PRECISION + 1))
#define REAL_PRECISION_FLOAT_IM(x)		cln::cl_float(cln::imagpart(x), cln::float_format(PRECISION + 1))
#define REAL_PRECISION_FLOAT(x)			cln::cl_float(x, cln::float_format(PRECISION + 1))
#define MIN_PRECISION_FLOAT_RE(x)		cln::cl_float(cln::realpart(x), cln::float_format(cln::float_format_lfloat_min + 1))
#define MIN_PRECISION_FLOAT_IM(x)		cln::cl_float(cln::imagpart(x), cln::float_format(cln::float_format_lfloat_min + 1))
#define MIN_PRECISION_FLOAT(x)			cln::cl_float(x, cln::float_format(cln::float_format_lfloat_min + 1))

#define CANNOT_FLOAT(x, p)			(((cln::plusp(cln::realpart(x)) && (cln::realpart(x) > cln::cl_float(cln::most_positive_float(cln::float_format(p)), cln::default_float_format) || cln::realpart(x) < cln::cl_float(cln::least_positive_float(cln::float_format(p)), cln::default_float_format))) || (cln::minusp(cln::realpart(x)) && (cln::realpart(x) < cln::cl_float(cln::most_negative_float(cln::float_format(p)), cln::default_float_format) || cln::realpart(x) > cln::cl_float(cln::least_negative_float(cln::float_format(p)), cln::default_float_format)))))

using namespace cln;

/*
void cln::cl_abort() {
	CALCULATOR->error(true, "CLN Error: see terminal output (probably too large or small floating point number)", NULL);
	if(CALCULATOR->busy()) {
		CALCULATOR->abort_this();
	} else {
		exit(0);
	}
}
*/

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

string printCL_I(cl_I integ, int base = 10, bool display_sign = true, BaseDisplay base_display = BASE_DISPLAY_NORMAL, bool lower_case = false) {
	bool is_neg = minusp(integ);
	if(is_neg) {
		integ = -integ;
	}
	if(base == BASE_ROMAN_NUMERALS) {
		if(!zerop(integ) && integ < 10000) {
			string str;
			int value = 0;
			try {
				value = cl_I_to_int(integ);
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}			
			if(is_neg) {
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
		} else if(!zerop(integ)) {
			CALCULATOR->error(false, _("Cannot display numbers greater than 9999 or less than -9999 as roman numerals."), NULL);
		}
		base = 10;
	}	
	
	string cl_str;
	cl_print_flags flags;
	flags.rational_base = base;
	
	if(integ > LLONG_MAX) {
		int i_step = 1000000;
		if(base < 8) i_step = 100;
		cl_I i_div = cln::expt_pos(cl_I(base), cl_I(i_step));		
		string str, stream_str;
		cl_I_div_t i_remdiv;
		while(true) {
			ostringstream stream;			
			try {
				i_remdiv = cln::floor2(integ, i_div);
				if(!zerop(i_remdiv.remainder)) {
					cln::print_integer(stream, flags, i_remdiv.remainder);
				}
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}			
			integ = i_remdiv.quotient;
			if(cln::zerop(integ)) {
				cl_str = stream.str() + cl_str;
				break;
			} else {
				str = "";
				stream_str = stream.str();
				str.resize((size_t) i_step - stream_str.length(), '0');
				str += stream_str;
				cl_str = str + cl_str;
			}
			if(CALCULATOR->aborted()) {
				return CALCULATOR->abortedMessage();
			}
		}
	} else {
		ostringstream stream;
		try {
			cln::print_integer(stream, flags, integ);
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		}
		cl_str = stream.str();
	}
	
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
	return format_number_string(cl_str, base, base_display, is_neg && display_sign);
}

Number::Number() {
	clear();
}
Number::Number(string number, const ParseOptions &po) {
	set(number, po);
}
Number::Number(int numerator, int denominator, int exp_10) {
	set(numerator, denominator, exp_10);
}
Number::Number(const Number &o) {
	set(o);
}
Number::~Number() {
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
	cl_I num = 0;
	cl_I den = 1;
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
	int readprec = 0;
	bool numbers_started = false, minus = false, in_decimals = false, b_cplx = false, had_nonzero = false;
	for(size_t index = 0; index < number.size(); index++) {
		if(number[index] >= '0' && ((base >= 10 && number[index] <= '9') || (base < 10 && number[index] < '0' + base))) {
			num = num * base;
			if(number[index] != '0') {
				num = num + number[index] - '0';
				if(!had_nonzero) readprec = 0;
				had_nonzero = true;
			}
			if(in_decimals) {
				den = den * base;
			}
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'a' && number[index] < 'a' + base - 10) {
			num = num * base;
			num = num + (number[index] - 'a' + 10);
			if(in_decimals) {
				den = den * base;
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(base > 10 && number[index] >= 'A' && number[index] < 'A' + base - 10) {
			num = num * base;
			num = num + (number[index] - 'A' + 10);
			if(in_decimals) {
				den = den * base;
			}
			if(!had_nonzero) readprec = 0;
			had_nonzero = true;
			readprec++;
			numbers_started = true;
		} else if(number[index] == 'E' && base <= 10) {
			index++;
			numbers_started = false;
			bool exp_minus = false;
			cl_I exp;
			while(index < number.size()) {
				if(number[index] >= '0' && number[index] <= '9') {				
					exp = exp * 10;
					exp = exp + number[index] - '0';
					numbers_started = true;
				} else if(!numbers_started && number[index] == '-') {
					exp_minus = !exp_minus;
				}
				index++;
			}
			try {
				if(exp_minus) {
					cl_I cl10 = 10;
					exp = cln::abs(exp);
					den = den * expt_pos(cl10, exp);
				} else {
					cl_I cl10 = 10;
					num = num * expt_pos(cl10, exp);
				}
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
				/*vector<cl_I> nums;
				nums.push_back(num);
				num = 0;
				for(index++; index < number.size(); index++) {
					if(number[index] >= '0' && number[index] <= '9') {
						num = num * 10;
						num = num + number[index] - '0';
					} else if(number[index] == ':') {
						nums.push_back(num);
						num = 0;
					} else if(number[index] == 'E')	{
						index--;
						break;
					} else if(number[index] == '.') {
						CALCULATOR->error(true, _("Decimal point in sexagesimal number treated as \':\'."), NULL);
						nums.push_back(num);
						num = 0;
					} else if(number[index] == 'i') {
						b_cplx = true;
					}
				}
				for(int i = nums.size() - 1; i >= 0; i--) {
					den = den * 60;
					nums[i] = nums[i] * den;
					num = num + nums[i];
				}*/
			}
		} else if(!numbers_started && number[index] == '-') {
			minus = !minus;
		} else if(number[index] == 'i') {
			b_cplx = true;
		} else if(number[index] != ' ') {
			CALCULATOR->error(true, _("Character \'%c\' was ignored in the number \"%s\" with base %s."), number[index], number.c_str(), i2s(base).c_str(), NULL);
		}
	}
	if(minus) num = -num;
	if(b_cplx) {
		value = cln::complex(0, num / den);
	} else {
		value = num / den;
	}
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
void Number::set(int numerator, int denominator, int exp_10) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	i_precision = -1;
	value = numerator;
	if(denominator) {
		value = value / denominator;
	}
	if(exp_10 != 0) {
		exp10(exp_10);
	}	
}
void Number::setFloat(double d_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = true;
	value = d_value;
	i_precision = 8;
}
void Number::setInternal(const cl_N &cln_value) {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	value = cln_value;
	i_precision = -1;
	testApproximate();
}
void Number::setImaginaryPart(const Number &o) {
	value = cln::complex(cln::realpart(value), cln::realpart(o.internalNumber()));
	testApproximate();
}
void Number::setImaginaryPart(int numerator, int denominator, int exp_10) {
	Number o(numerator, denominator, exp_10);
	setImaginaryPart(o);
}
void Number::set(const Number &o) {
	b_inf = o.isInfinity(); 
	b_pinf = o.isPlusInfinity(); 
	b_minf = o.isMinusInfinity();
	value = o.internalNumber();
	b_approx = o.isApproximate();
	i_precision = o.precision();
}
void Number::setInfinity() {
	b_inf = true;
	b_pinf = false;
	b_minf = false;
	b_approx = false;
	value = 0;
	i_precision = -1;
}
void Number::setPlusInfinity() {
	b_inf = false;
	b_pinf = true;
	b_minf = false;
	b_approx = false;
	value = 0;
	i_precision = -1;
}
void Number::setMinusInfinity() {
	b_inf = false; 
	b_pinf = false;
	b_minf = true;
	b_approx = false;
	value = 0;
	i_precision = -1;
}

void Number::clear() {
	b_inf = false; b_pinf = false; b_minf = false; b_approx = false;
	value = 0;
	i_precision = -1;
}

const cl_N &Number::internalNumber() const {
	return value;
}

double Number::floatValue() const {
	double d = 0.0;
	try {
		d = double_approx(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return d;
}
int Number::intValue(bool *overflow) const {
	cl_I i;
	try {
		i = cln::round1(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	if(i > long(INT_MAX)) {
		if(overflow) *overflow = true;
		return INT_MAX;
	} else if(i < long(INT_MIN)) {
		if(overflow) *overflow = true;
		return INT_MIN;
	}
	return cl_I_to_int(i);
}

bool Number::isApproximate() const {
	return b_approx || isApproximateType();	
}
bool Number::isApproximateType() const {
	return !isInfinite() && (!cln::instanceof(cln::realpart(value), cln::cl_RA_ring) || (isComplex() && !cln::instanceof(cln::imagpart(value), cln::cl_RA_ring)));	
}
void Number::setApproximate(bool is_approximate) {
	if(!isInfinite() && is_approximate != isApproximate()) {
		if(is_approximate) {
			//value = cln::complex(cln::cl_float(cln::realpart(value)), cln::cl_float(cln::imagpart(value)));
			//removeFloatZeroPart();
			i_precision = PRECISION;
			b_approx = true;
		} else {
			if(isApproximateType()) {
				value = cln::complex(cln::rational(cln::realpart(value)), cln::rational(cln::imagpart(value)));
			}
			i_precision = -1;
			b_approx = false;
		}
	}
}

int Number::precision() const {
	return i_precision;
}
void Number::setPrecision(int prec) {
	i_precision = prec;
	if(i_precision > 0) b_approx = true;
}

bool Number::isUndefined() const {
	return false;
}
bool Number::isInfinite() const {
	return b_pinf || b_minf || b_inf;
}
bool Number::isInfinity() const {
	return b_inf;
}
bool Number::isPlusInfinity() const {
	return b_pinf;
}
bool Number::isMinusInfinity() const {
	return b_minf;
}

Number Number::realPart() const {
	if(isInfinite()) return *this;
	Number real_part;
	real_part.setInternal(cln::realpart(value));
	return real_part;
}
Number Number::imaginaryPart() const {
	if(isInfinite()) return Number();
	Number imag_part;
	imag_part.setInternal(cln::imagpart(value));
	return imag_part;
}
Number Number::numerator() const {
	Number num;
	num.setInternal(cln::numerator(cln::rational(cln::realpart(value))));
	return num;
}
Number Number::denominator() const {
	Number den;
	den.setInternal(cln::denominator(cln::rational(cln::realpart(value))));
	return den;
}
Number Number::complexNumerator() const {
	Number num;
	num.setInternal(cln::numerator(cln::rational(cln::imagpart(value))));
	return num;
}
Number Number::complexDenominator() const {
	Number den;
	den.setInternal(cln::denominator(cln::rational(cln::imagpart(value))));
	return den;
}

void Number::operator = (const Number &o) {set(o);}
void Number::operator -- (int) {value = cln::minus1(value);}
void Number::operator ++ (int) {value = cln::plus1(value);}
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
	try {
		value = cln::logand(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitOr(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	try {
		value = cln::logior(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitXor(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	try {
		value = cln::logxor(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::bitNot() {
	if(!isInteger()) return false;
	try {
		value = cln::lognot(cln::numerator(cln::rational(cln::realpart(value))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return true;
}
bool Number::bitEqv(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	try {
		value = cln::logeqv(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftLeft(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	cln::cl_I intval;
	try {
		intval = cln::numerator(cln::rational(cln::realpart(value)));
		intval << cln::numerator(cln::rational(cln::realpart(o.internalNumber())));
		value = intval;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shiftRight(const Number &o) {
	if(!o.isInteger() || !isInteger() || o.isNegative()) return false;
	cln::cl_I intval;
	try {
		intval = cln::numerator(cln::rational(cln::realpart(value)));
		intval >> cln::numerator(cln::rational(cln::realpart(o.internalNumber())));
		value = intval;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::shift(const Number &o) {
	if(!o.isInteger() || !isInteger()) return false;
	try {
		value = cln::ash(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	setPrecisionAndApproximateFrom(o);
	return true;
}

bool Number::hasRealPart() const {
	if(isInfinite()) return true;
	bool b = false;
	try {
		b = !cln::zerop(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return b;
}
bool Number::hasImaginaryPart() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = !cln::zerop(cln::imagpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		bool b = false;
		try {
			if(PRECISION < cln::float_format_lfloat_min) {
				b = cln::zerop(cln::truncate2(MIN_PRECISION_FLOAT_RE(value)).remainder);
			} else {
				b = cln::zerop(cln::truncate2(REAL_PRECISION_FLOAT_RE(value)).remainder);
			}
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		}
		if(b) {
			cln::cl_N new_value;
			try {
				new_value = cln::round1(cln::realpart(value));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
	return cln::denominator(cln::rational(cln::realpart(value))) == 1;
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
		cl_R real_value = cln::realpart(value);
		return real_value < 1 && real_value > -1;
	}
	return false; 
}
bool Number::isZero() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = cln::zerop(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return b;
}
bool Number::isOne() const {
	if(isInfinite()) return false;
	return value == 1;
}
bool Number::isTwo() const {
	if(isInfinite()) return false;
	return value == 2;
}
bool Number::isI() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = cln::zerop(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return b && cln::imagpart(value) == 1;
}
bool Number::isMinusOne() const {
	if(isInfinite()) return false;
	return value == -1;
}
bool Number::isMinusI() const {
	if(isInfinite()) return false;
	bool b = false;
	try {
		b = cln::zerop(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return b && cln::imagpart(value) == -1;
}
bool Number::isNegative() const {
	return b_minf || (!isInfinite() && !isComplex() && cln::minusp(cln::realpart(value)));
}
bool Number::isNonNegative() const {
	return b_pinf || (!isInfinite() && !isComplex() && !cln::minusp(cln::realpart(value)));
}
bool Number::isPositive() const {
	return b_pinf || (!isInfinite() && !isComplex() && cln::plusp(cln::realpart(value)));
}
bool Number::isNonPositive() const {
	return b_minf || (!isInfinite() && !isComplex() && !cln::plusp(cln::realpart(value)));
}
bool Number::realPartIsNegative() const {
	return b_minf || (!isInfinite() && cln::minusp(cln::realpart(value)));
}
bool Number::realPartIsPositive() const {
	return b_pinf || (!isInfinite() && cln::plusp(cln::realpart(value)));
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
	if(isApproximateType() && !isComplex()) {
		bool b_zero = false;
		try {
			if(PRECISION < cln::float_format_lfloat_min) {
				b_zero = MIN_PRECISION_FLOAT_RE(value + 1) == cln::cl_float(1, cln::float_format(cln::float_format_lfloat_min + 1));
			} else {
				b_zero = REAL_PRECISION_FLOAT_RE(value + 1) == cln::cl_float(1, cln::float_format(PRECISION + 1));
			}
		} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		}
		return b_zero;
	}
	return false;
}
bool Number::equals(const Number &o) const {
	if(b_inf) return false;
	if(b_pinf) return false;
	if(b_minf) return false;
	if(o.isInfinite()) return false;
	return value == o.internalNumber();
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		int i = cln::compare(cln::realpart(o.internalNumber()), cln::realpart(value));
		if(i == 0) return COMPARISON_RESULT_EQUAL;
		else if(i == -1) return COMPARISON_RESULT_LESS;
		else if(i == 1) return COMPARISON_RESULT_GREATER;
		return COMPARISON_RESULT_UNKNOWN;
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
		int i = cln::compare(cln::realpart(o.internalNumber()), cln::realpart(value));
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
	return cln::realpart(value) > cln::realpart(o.internalNumber());
}
bool Number::isLessThan(const Number &o) const {
	if(o.isMinusInfinity() || o.isInfinity() || b_inf || b_pinf) return false;
	if(b_minf || o.isPlusInfinity()) return true;
	if(isComplex() || o.isComplex()) return false;
	return cln::realpart(value) < cln::realpart(o.internalNumber());
}
bool Number::isGreaterThanOrEqualTo(const Number &o) const {
	if(b_inf || o.isInfinity()) return false;
	if(b_minf) return o.isMinusInfinity();
	if(b_pinf) return true;
	if(!isComplex() && !o.isComplex()) {
		return cln::realpart(value) >= cln::realpart(o.internalNumber());
	}
	return false;
}
bool Number::isLessThanOrEqualTo(const Number &o) const {
	if(b_inf || o.isInfinity()) return false;
	if(b_pinf) return o.isPlusInfinity();
	if(b_minf) return true;
	if(!isComplex() && !o.isComplex()) {
		return cln::realpart(value) <= cln::realpart(o.internalNumber());
	}
	return false;
}
bool Number::isEven() const {
	return isInteger() && cln::evenp(cln::numerator(cln::rational(cln::realpart(value))));
}
bool Number::denominatorIsEven() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && cln::evenp(cln::denominator(cln::rational(cln::realpart(value))));
}
bool Number::denominatorIsTwo() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && cln::denominator(cln::rational(cln::realpart(value))) == 2;
}
bool Number::numeratorIsEven() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && cln::evenp(cln::numerator(cln::rational(cln::realpart(value))));
}
bool Number::numeratorIsOne() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && cln::numerator(cln::rational(cln::realpart(value))) == 1;
}
bool Number::numeratorIsMinusOne() const {
	return !isInfinite() && !isComplex() && !isApproximateType() && cln::numerator(cln::rational(cln::realpart(value))) == -1;
}
bool Number::isOdd() const {
	return isInteger() && cln::oddp(cln::numerator(cln::rational(cln::realpart(value))));
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
	if(isApproximateType() || o.isApproximateType()) {
		if(equalsApproximately(-o, EQUALS_PRECISION_DEFAULT)) {
			value = 0;
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	cln::cl_N new_value;
	try {
		new_value = value + o.internalNumber();
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	if(isApproximateType() || o.isApproximateType()) {
		if(equalsApproximately(o, EQUALS_PRECISION_DEFAULT)) {
			value = 0;
			setPrecisionAndApproximateFrom(o);
			return true;
		}
	}
	cln::cl_N new_value;
	try {
		new_value = value - o.internalNumber();
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value;
	try {
		new_value = value * o.internalNumber();
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value;
	try {
		new_value = value / o.internalNumber();
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value;
	try {
		new_value = cln::recip(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value = value;
	bool neg = false;
	if(isNegative() && !o.isComplex() && !o.isApproximateType() && !o.numeratorIsEven() && !o.denominatorIsEven()) {
		neg = true;
		new_value = cln::abs(new_value);
	}	
	cln::cl_RA dmax = 1;
	dmax = dmax / 10;

	cln::cl_R v_log10 = cln::realpart(value);
	cln::cl_R o_log10 = cln::realpart(o.internalNumber());
	if(!zerop(cln::imagpart(value))) {
		if(cln::abs(cln::imagpart(o.internalNumber())) > cln::abs(o_log10)) {
			if(!cln::zerop(v_log10) && !cln::zerop(o_log10) && v_log10 != 1 && v_log10 != -1 && o_log10 != 1 && o_log10 != -1) {
				if(v_log10 != 1) v_log10 = cln::abs(cln::log(v_log10, 10));
				if(v_log10 != 0) {
					o_log10 = cln::log(o_log10 * v_log10, 10);
					if(o_log10 > 4 && CALCULATOR->aborted()) return false;
					if(o_log10 > 6) {
						CALCULATOR->error(false, _("Extreme exponentiation was not calculated."), NULL);
						return false;
					}
				}
			}
			o_log10 = cln::imagpart(o.internalNumber());
			v_log10 = cln::imagpart(value);
		} else if(cln::abs(cln::imagpart(value)) > cln::abs(v_log10)) {
			v_log10 = cln::imagpart(value);
		}
	}
	if(!cln::zerop(v_log10) && !cln::zerop(o_log10) && v_log10 != 1 && v_log10 != -1 && o_log10 != 1 && o_log10 != -1) {
		if(v_log10 != 1) v_log10 = cln::abs(cln::log(v_log10, 10));
		if(v_log10 != 0) {
			o_log10 = cln::log(o_log10 * v_log10, 10);
			if(o_log10 > 4 && CALCULATOR->aborted()) return false;
			if(o_log10 > 6) {
				CALCULATOR->error(false, _("Extreme exponentiation was not calculated."), NULL);
				return false;
			}
		}
	}

	if(o.isRational() && isRational() && (!try_exact || (cln::abs(new_value) <= 1 + dmax && cln::abs(new_value) >= 1 - dmax)) && new_value != 1 && new_value != -1 && (cln::numerator(cln::rational(cln::realpart(o.internalNumber()))) > 10000 || cln::numerator(cln::rational(cln::realpart(o.internalNumber()))) < -10000)) {
		try {
			new_value = cln::expt(cln::cl_float(cln::realpart(new_value)), cln::cl_float(cln::realpart(o.internalNumber())));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	} else {
		try {
			new_value = cln::expt(new_value, o.internalNumber());
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	}
	if(neg) {
		try {
			new_value = -new_value;
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	}
	
	value = new_value;
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
	cln::cl_N new_value;
	try {
		new_value = cln::square(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}

bool Number::negate() {
	if(isInfinite()) {
		b_pinf = !b_pinf;
		b_minf = !b_minf;
		return true;
	}
	cln::cl_N new_value;
	try {
		new_value = -value;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
void Number::setNegative(bool is_negative) {
	if(!isZero() && minusp(cln::realpart(value)) != is_negative) {
		if(isInfinite()) {b_pinf = !b_pinf; b_minf = !b_minf; return;}
		try {
			value = cln::complex(-cln::realpart(value), cln::imagpart(value));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		}
	}
}
bool Number::abs() {
	if(isInfinite()) {
		setPlusInfinity();
		return true;
	}
	value = cln::abs(value);
	return true;
}
bool Number::signum() {
	if(isInfinite()) return false;
	value = cln::signum(value);
	return true;
}
bool Number::round() {
	if(isInfinite() || isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::round1(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	if(b_approx) {
		if(isInteger()) {
			bool b_zero = false;
			try {
				b_zero = cln::zerop(cln::rem(cln::realpart(value), 10));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			if(!b_zero) {
				i_precision = -1;
				b_approx = false;
			}
		} else {
			i_precision = -1;
			b_approx = false;
		}
	}
	return true;
}
bool Number::floor() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	cln::cl_N new_value;
	try {
		new_value = cln::floor1(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::ceil() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	cln::cl_N new_value;
	try {
		new_value = cln::ceiling1(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::trunc() {
	if(isInfinite() || isComplex()) return false;
	//if(b_approx && !isInteger()) b_approx = false;
	cln::cl_N new_value;
	try {
		new_value = cln::truncate1(cln::realpart(value));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::frac() {
	if(isInfinite() || isComplex()) return false;
	cln::cl_N new_value;
	try {
		cl_N whole_value = cln::truncate1(cln::realpart(value));
		new_value = value - whole_value;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::rem(const Number &o) {
	if(isInfinite() || o.isInfinite()) return false;
	if(isComplex() || o.isComplex()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::rem(cln::realpart(value), cln::realpart(o.internalNumber()));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::irem(const Number &o, Number &q) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	try {
		const cln::cl_I_div_t rem_quo = cln::truncate2(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		q.setInternal(rem_quo.quotient);
		value = rem_quo.remainder;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return true;
}
bool Number::iquo(const Number &o) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	cln::cl_N new_value;
	try {
		new_value = cln::truncate1(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::iquo(const Number &o, Number &r) {
	if(o.isZero()) return false;
	if(!isInteger() || !o.isInteger()) return false;
	try {
		const cln::cl_I_div_t rem_quo = cln::truncate2(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		r.setInternal(rem_quo.remainder);
		value = rem_quo.quotient;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
	}
	return true;
}
bool Number::isqrt() {
	if(isInteger()) {
		cln::cl_N new_value;
		try {
			cln::cl_I iroot;
			cln::isqrt(cln::numerator(cln::rational(cln::realpart(value))), &iroot);
			new_value = iroot;
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
		value = new_value;
		return true;
	}
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
	setInternal(cln::exp1());
}
void Number::pi() {
	setInternal(cln::pi());
}
void Number::catalan() {
	setInternal(cln::catalanconst());
}
void Number::euler() {
	setInternal(cln::eulerconst());
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
	int i = intValue(&overflow);
	if(overflow || i > 100000) {
		CALCULATOR->error(true, _("Cannot handle an argument (s) that large for Riemann Zeta."), NULL);
		return false;
	}
	cln::cl_N new_value;
	try {
		new_value = cln::zeta(i); 
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}			

bool Number::sin() {
	if(isInfinite()) return false;
	if(isZero()) return true;
	cln::cl_N new_value;
	try {
		new_value = cln::sin(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
	cln::cl_N new_value;
	try {
		new_value = cln::log(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value;
	if(!isApproximate() && !o.isApproximate() && isFraction()) {	
		try {		
			new_value = -cln::log(cln::recip(value), o.internalNumber());			
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	} else {
		try {
			new_value = cln::log(value, o.internalNumber());
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	}
	value = new_value;
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
	cln::cl_N new_value;
	try {
		new_value = cln::exp(value);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
	}
	cln::cl_RA wPrec;
	try {
		wPrec = cln::expt(cln::cl_I(10), -(PRECISION + 2));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
	cln::cl_N new_value;
	
	try {
		cl_I num = cln::numerator(cln::rational(cln::realpart(value)));
		cl_I num_o = cln::numerator(cln::rational(cln::realpart(o.internalNumber())));
		new_value = cln::gcd(num, num_o);
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	setPrecisionAndApproximateFrom(o);
	return true;
}
bool Number::lcm(const Number &o) {
	if(isInteger() && o.isInteger()) {
		cln::cl_N new_value;
		try {
			new_value = cln::lcm(cln::numerator(cln::rational(cln::realpart(value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
		value = new_value;
		return true;
	}
	return multiply(o);
}

cln::cl_I recfact(cln::cl_I start, cln::cl_I n) {
	cln::cl_I i;
	if(n <= 16) { 
		cln::cl_I r = start;
		for(i = start + 1; i < start + n; i++) r *= i;
		return r;
	}
	if(CALCULATOR->aborted()) return 0;
	i = cln::floor1(n, 2);
	return recfact(start, i) * recfact(start + i, n - i);
}
cln::cl_I recfact2(cln::cl_I start, cln::cl_I n) {
	cln::cl_I i;
	if(n <= 32) { 
		cln::cl_I r = start + n - 1;
		for(i = start + n - 3; i >= start; i -= 2) r *= i;
		return r;
	}
	if(CALCULATOR->aborted()) return 0;
	i = cln::floor1(n, 2);
	if(cln::evenp(i) != cln::evenp(n)) {
		return recfact2(start, i - 1) * recfact2(start + i - 1, n - i + 1);
	}
	return recfact2(start, i) * recfact2(start + i, n - i);
}
cln::cl_I recfactm(cln::cl_I start, cln::cl_I n, cln::cl_I m) {
	cln::cl_I i;
	if(n <= 16 * m) { 
		cln::cl_I r = start + n - 1;
		for(i = start + n - 1 - m; i >= start; i -= m) r *= i;
		return r;
	}
	if(CALCULATOR->aborted()) return 0;
	i = cln::floor1(n, 2);
	cln::cl_I im = cln::rem(i, m) - cln::rem(n, m);
	return recfactm(start, i - im, m) * recfactm(start + i - im, n - i + im, m);
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
	cln::cl_N new_value = value;
	try {
		new_value = recfact(1, cln::numerator(cln::rational(cln::realpart(new_value))));
		if(cln::zerop(new_value)) return false;
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
	return true;
}
bool Number::multiFactorial(const Number &o) {
	if(o.isOne()) return factorial();
	if(o.isTwo()) return doubleFactorial();
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
	cln::cl_N new_value = value;
	try {
		new_value = recfactm(1, cln::numerator(cln::rational(cln::realpart(new_value))), cln::numerator(cln::rational(cln::realpart(o.internalNumber()))));
		if(cln::zerop(new_value)) return false;
		/*cln::cl_I i = cln::numerator(cln::rational(cln::realpart(new_value)));
		cln::cl_I i_o = cln::numerator(cln::rational(cln::realpart(o.internalNumber())));
		i = i - i_o;
		for(; cln::plusp(i); i = i - i_o) {
			if(CALCULATOR->aborted()) return false;
			new_value = new_value * i;
		}*/
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
	cln::cl_N new_value = value;
	try {
		new_value = recfact2(1, cln::numerator(cln::rational(cln::realpart(new_value))));
		if(cln::zerop(new_value)) return false;
		/*cln::cl_I i = cln::numerator(cln::rational(cln::realpart(value)));
		cln::cl_I i2 = 2;
		i = i - i2;
		for(; cln::plusp(i); i = i - i2) {
			if(CALCULATOR->aborted()) return false;
			new_value = new_value * i;
		}*/
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	value = new_value;
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
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			return false;
		}
		if(im > long(INT_MAX) || ik > 100000) {
			ik = cln::minus1(ik);
			Number k_fac(k);
			try {
				k_fac.factorial();
				cl_I ithis = im;
				for(; !cln::zerop(ik); ik = cln::minus1(ik)) {
					if(CALCULATOR->aborted()) return false;
					im = cln::minus1(im);
					ithis = ithis * im;
				}
				new_value = ithis;
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				return false;
			}
			clear();
			value = new_value;
			divide(k_fac);
		} else {
			try {
				new_value = cln::binomial(cl_I_to_uint(im), cl_I_to_uint(ik));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
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
	cl_I inr;
	try {
		inr = cln::numerator(cln::rational(cln::realpart(value)));
	} catch(runtime_exception &e) {
		CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		return false;
	}
	if(inr == 1) {
		factors.push_back(Number(1, 1));
		return true;
	}
	if(inr == -1) {
		factors.push_back(Number(-1, 1));
		return true;
	}
	if(minusp(inr)) {
		inr = -inr;
		factors.push_back(Number(-1, 1));
	}
	size_t prime_index = 0;
	cl_I last_prime = 0;
	bool b = true;
	while(b) {
		if(CALCULATOR->aborted()) return false;
		b = false;
		cl_I facmax = cln::floor1(cln::sqrt(inr));
		for(; prime_index < NR_OF_PRIMES && PRIMES[prime_index] <= facmax; prime_index++) {
			bool b_zero = false;
			try {
				b_zero = cln::zerop(cln::mod(inr, PRIMES[prime_index]));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			if(b_zero) {
				try {
					inr = cln::exquo(inr, PRIMES[prime_index]);
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
				Number fac;
				fac.setInternal(PRIMES[prime_index]);
				factors.push_back(fac);
				b = true;
				break;
			}
		}
		if(prime_index == NR_OF_PRIMES) {
			last_prime = PRIMES[NR_OF_PRIMES - 1] + 2;
			prime_index++;
		}
		if(!b && prime_index > NR_OF_PRIMES) {
			while(!b && last_prime <= facmax) {
				bool b_zero = false;
				try {
					b_zero = cln::zerop(cln::mod(inr, last_prime));
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
				if(CALCULATOR->aborted()) return false;
				if(b_zero) {
					try {
						inr = cln::exquo(inr, last_prime);
					} catch(runtime_exception &e) {
						CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
					}
					b = true;
					Number fac;
					fac.setInternal(last_prime);
					factors.push_back(fac);
					break;
				}
				last_prime = last_prime + 2;
			}
		}
	}
	if(inr != 1) {
		Number fac;
		fac.setInternal(inr);
		factors.push_back(fac);
	}
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
	return printCL_I(cln::numerator(cln::rational(cln::realpart(value))), base, display_sign, base_display, lower_case);
}
string Number::printDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printCL_I(cln::denominator(cln::rational(cln::realpart(value))), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryNumerator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printCL_I(cln::numerator(cln::rational(cln::imagpart(value))), base, display_sign, base_display, lower_case);
}
string Number::printImaginaryDenominator(int base, bool display_sign, BaseDisplay base_display, bool lower_case) const {
	return printCL_I(cln::denominator(cln::rational(cln::imagpart(value))), base, display_sign, base_display, lower_case);
}

string Number::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
	if(isApproximateType() && !isInfinite() && !isComplex()) {
		if((PRECISION < cln::float_format_lfloat_min && cln::zerop(cln::truncate2(MIN_PRECISION_FLOAT_RE(value)).remainder))
		|| (PRECISION >= cln::float_format_lfloat_min && cln::zerop(cln::truncate2(REAL_PRECISION_FLOAT_RE(value)).remainder))) {
			Number nr_copy(*this);
			nr_copy.testInteger();
			return nr_copy.print(po, ips);
		}
	}
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
	int min_decimals = po.min_decimals;
	if(min_decimals < 0 || !po.use_min_decimals) min_decimals = 0;
	if(min_decimals > po.max_decimals && po.use_max_decimals && po.max_decimals >= 0) {
		min_decimals = po.max_decimals;
	}
	if(po.base <= 1 && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_TIME) base = 10;
	else if(po.base > 36 && po.base != BASE_SEXAGESIMAL) base = 36;
	else base = po.base;
	if(isApproximateType() && base == BASE_ROMAN_NUMERALS) base = 10;
	int precision = PRECISION;
	if(b_approx && i_precision > 0 && i_precision < PRECISION) precision = i_precision;
	if(po.restrict_to_parent_precision && ips.parent_precision > 0 && ips.parent_precision < precision) precision = ips.parent_precision;
	bool approx = isApproximate() || (ips.parent_approximate && po.restrict_to_parent_precision);
	if(isComplex()) {
		bool bre = hasRealPart();
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
		if(ips.num) *ips.num = str;
	} else if(isInteger()) {
		cl_I ivalue = cln::numerator(cln::rational(cln::realpart(value)));
		bool neg = cln::minusp(ivalue);
		bool rerun = false;
		bool exact = true;

		integer_rerun:

		string mpz_str = printCL_I(ivalue, base, false, BASE_DISPLAY_NONE, po.lower_case_numbers);
		if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
		
		int length = mpz_str.length();
		
		int expo = 0;
		if(base == 10) {
			if(mpz_str.length() > 0 && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				expo = length - 1;
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
		int decimals = expo;
		int nondecimals = length - decimals;
		
		bool dp_added = false;
		bool b_zero = true;

		try {
			b_zero = cln::zerop(ivalue);
		} catch(runtime_exception &e) {
			CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
		}

		if(!rerun && !b_zero) {
			int precision2 = precision;
			if(base != 10) {
				Number precmax(10);
				precmax.raise(precision);
				precmax--;
				precmax.log(base);
				precmax.floor();
				precision2 = precmax.intValue();
			}
			if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals + nondecimals < precision2) && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
				try {
					cln::cl_RA_div_t div = cln::floor2(ivalue / cln::expt_pos(cln::cl_I(base), -(po.max_decimals - expo)));
					if(!cln::zerop(div.remainder)) {
						ivalue = div.quotient;
						if(po.round_halfway_to_even && cln::evenp(ivalue)) {
							if(div.remainder * base > cln::cl_I(base) / cln::cl_I(2)) {
								ivalue = cln::plus1(ivalue);
							}
						} else {
							if(div.remainder * base >= cln::cl_I(base) / cln::cl_I(2)) {
								ivalue = cln::plus1(ivalue);
							}
						}
						ivalue = ivalue * cln::expt_pos(cln::cl_I(base), -(po.max_decimals - expo));
						exact = false;
						rerun = true;
						goto integer_rerun;
					}
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
			} else if(precision2 < length && (approx || (decimals > min_decimals && (po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)))) {
				try {
					cln::cl_RA v = ivalue;
					precision2 = length - precision2;
					if(!approx && length - (min_decimals + nondecimals) < precision2) precision2 = length - (min_decimals + nondecimals);
					
					int p2_cd = precision2;
					
					cln::cl_I i_exp;

					long int p2_cd_min = 10000;
					while(p2_cd_min >= 1000) {
						if(p2_cd > p2_cd_min) {
							i_exp = cln::expt_pos(cln::cl_I(base), p2_cd_min);
							while(p2_cd > p2_cd_min) {
								v = v / i_exp;
								p2_cd -= p2_cd_min;
								if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
							}
						}
						p2_cd_min = p2_cd_min / 10;
					}
					
					i_exp = cln::expt_pos(cln::cl_I(base), p2_cd);
					v = v / i_exp;
					cln::cl_RA_div_t div = cln::floor2(v);

					if(!cln::zerop(div.remainder)) {
						ivalue = div.quotient;
						if(po.round_halfway_to_even && cln::evenp(ivalue)) {
							if(div.remainder * base > cln::cl_I(base) / cln::cl_I(2)) {
								ivalue = cln::plus1(ivalue);
							}
						} else {
							if(div.remainder * base >= cln::cl_I(base) / cln::cl_I(2)) {
								ivalue = cln::plus1(ivalue);
							}
						}
						ivalue = ivalue * cln::expt_pos(cln::cl_I(base), precision2);
						exact = false;
						rerun = true;
						goto integer_rerun;
					}
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
			}
		}
		decimals = 0;
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
			if(pos + 1 < (int) str.length()) {
				decimals -= str.length() - (pos + 1);
				str = str.substr(0, pos + 1);
			}
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
		if(po.show_ending_zeroes && (!exact || approx) && (!po.use_max_decimals || po.max_decimals < 0 || po.max_decimals > decimals)) {
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
	} else {
		if(base != BASE_ROMAN_NUMERALS && (isApproximateType() || po.number_fraction_format == FRACTION_DECIMAL || po.number_fraction_format == FRACTION_DECIMAL_EXACT)) {
			cln::cl_I num, d, remainder = 0, remainder2 = 0, exp = 0;
			cln::cl_I_div_t div;
			try {
				d = cln::denominator(cln::rational(cln::realpart(value)));
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}			
			bool neg = cln::minusp(cln::realpart(value));
			try {
				if(neg) {
					num = -cln::numerator(cln::rational(cln::realpart(value)));
				} else {
					num = cln::numerator(cln::rational(cln::realpart(value)));
				}
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			try {
				div = cln::truncate2(num, d);
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			try {
				remainder = div.remainder;
				num = div.quotient;
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			bool exact = false;
			try {
				exact = cln::zerop(remainder);
			} catch(runtime_exception &e) {
				CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
			}
			vector<cln::cl_I> remainders;
			if(base != 10) {
				Number precmax(10);
				precmax.raise(precision);
				precmax--;
				precmax.log(base);
				precmax.floor();
				precision = precmax.intValue();
			}
			bool started = false;
			int expo = 0;
			int precision2 = precision;
			bool num_zero = cln::zerop(num);
			if(!num_zero) {
				str = printCL_I(num, base, true, BASE_DISPLAY_NONE);
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				
				int length = str.length();
				if(base != 10) {
					expo = 0;
				} else {
					expo = length - 1;
					if(po.min_exp == EXP_PRECISION) {
						if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
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
				int decimals = expo;
				int nondecimals = length - decimals;
				precision2 -= length;
				if(!approx && min_decimals + nondecimals > precision) precision2 = (min_decimals + nondecimals) - length;
				try {
					if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals && (!approx || po.max_decimals - decimals < precision2)) {
						cln::cl_R_div_t divr = cln::floor2(cln::realpart(value) / cln::expt_pos(cln::cl_I(base), -(po.max_decimals - decimals)));
						if(!cln::zerop(divr.remainder)) {
							num = divr.quotient;
							if(po.round_halfway_to_even && cln::evenp(num)) {
								if(divr.remainder * base > cln::cl_I(base) / cln::cl_I(2)) {
									num = cln::plus1(num);
								}
							} else {
								if(divr.remainder * base >= cln::cl_I(base) / cln::cl_I(2)) {
									num = cln::plus1(num);
								}
							}
							num = num * cln::expt_pos(cln::cl_I(base), -(po.max_decimals - expo));
							exact = false;
							if(neg) num = -num;
						}
						remainder = 0;
					} else if(precision2 < 0 && (approx || decimals > min_decimals)) {
						cln::cl_R_div_t divr = cln::floor2(cln::realpart(value) / cln::expt_pos(cln::cl_I(base), -precision2));
						if(!cln::zerop(divr.remainder)) {
							num = divr.quotient;
							if(po.round_halfway_to_even && cln::evenp(num)) {
								if(divr.remainder * base > cln::cl_I(base) / cln::cl_I(2)) {
									num = cln::plus1(num);
								}
							} else {
								if(divr.remainder * base >= cln::cl_I(base) / cln::cl_I(2)) {
									num = cln::plus1(num);
								}
							}
							num = num * cln::expt_pos(cln::cl_I(base), -precision2);
							exact = false;
							if(neg) num = -num;
						}
						remainder = 0;
					}
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}	
				started = true;
				if(po.use_max_decimals && po.max_decimals >= 0 && precision2 > po.max_decimals - decimals) precision2 = po.max_decimals - decimals;
			}
			bool try_infinite_series = po.indicate_infinite_series && !isApproximateType();
			cl_I remainder_bak = remainder, num_bak = num;
			
			bool rerun = false;
			
			rational_rerun:

			bool infinite_series = false;
			int l10 = 0;
			if(rerun) {
				num = num_bak;
				remainder = remainder_bak;
			}
			while(!exact && precision2 > 0) {
				if(try_infinite_series) {
					remainders.push_back(remainder);
				}
				try {
					remainder = remainder * base;
					div = cln::truncate2(remainder, d);
					remainder2 = div.remainder;
					remainder = div.quotient;
					exact = cln::zerop(remainder2);
					if(!started) {
						started = !cln::zerop(remainder);
					}
					if(started) {
						num = num * base;	
						num = num + remainder;
					}
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				l10++;
				remainder = remainder2;				
				if(try_infinite_series && !exact) {
					for(size_t i = 0; i < remainders.size(); i++) {
						if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
						if(remainders[i] == remainder) {
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
			remainders.clear();
			if(!exact && !infinite_series) {
				try {
					remainder = remainder * base;
					div = cln::truncate2(remainder, d);
					remainder2 = div.remainder;
					remainder = div.quotient;
					if(po.round_halfway_to_even && cln::evenp(num) && cln::zerop(remainder2)) {
						if(remainder > cl_I(base) / cl_I(2)) {
							num = cln::plus1(num);
						}
					} else {
						if(remainder >= cl_I(base) / cl_I(2)) {
							num = cln::plus1(num);
						}
					}
				} catch(runtime_exception &e) {
					CALCULATOR->error(true, _("CLN Exception: %s"), e.what(), NULL);
				}
			}
			if(!exact && !infinite_series) {
				if(po.number_fraction_format == FRACTION_DECIMAL_EXACT && !isApproximate()) {
					PrintOptions po2 = po;
					po2.number_fraction_format = FRACTION_FRACTIONAL;
					return print(po2, ips);
				}
				if(po.is_approximate) *po.is_approximate = true;
			}
			str = printCL_I(num, base, true, BASE_DISPLAY_NONE, po.lower_case_numbers);
			if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
			if(!rerun && base == 10) {
				expo = str.length() - l10 - 1;
				if(po.min_exp == EXP_PRECISION) {
					if((expo > -precision && expo < precision) || (expo < 3 && expo > -3 && PRECISION >= 3)) { 
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
			if(!rerun && num_zero && expo <= 0 && po.use_max_decimals && po.max_decimals >= 0 && precision > po.max_decimals - l10 - expo + (int) str.length()) {
				precision2 = po.max_decimals + (int) str.length() - l10 - expo;
				rerun = true;
				exact = false;
				started = false;
				goto rational_rerun;
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
				int decimals = str.length() - l10 - 1;
				if(!exact && po.show_ending_zeroes) {
					if(po.use_max_decimals && po.max_decimals >= 0 && decimals > po.max_decimals) {
						l2 = decimals - po.max_decimals;
					} else {
						l2 = 0;
					}
				}
				if(l2 > 0 && !infinite_series) {
					if(min_decimals > 0) {
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
			num.setInternal(cln::numerator(cln::rational(cln::realpart(value))));
			den.setInternal(cln::denominator(cln::rational(cln::realpart(value))));
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
	}
	return str;
}


