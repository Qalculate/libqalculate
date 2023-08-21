#include <libqalculate/qalculate.h>
#include "support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::cerr;

int has_not_a_comparison() {
	if(!CALCULATOR->message()) return 0;
	while(true) {
		if(CALCULATOR->message()->message() == _("The calculation has been forcibly terminated. Please restart the application and report this as a bug.")) return -1;
		//if(CALCULATOR->message()->message() == "A") return 1;
		if(!CALCULATOR->nextMessage()) break;
	}
	return 0;
}
bool display_errors(bool show_only_errors = false) {
	if(!CALCULATOR->message()) return false;
	bool b_ret = false;
	while(true) {
		MessageType mtype = CALCULATOR->message()->type();
		if(!show_only_errors || mtype == MESSAGE_ERROR) {
			if(mtype == MESSAGE_ERROR) cout << "error: ";
			else if(mtype == MESSAGE_WARNING) cout << "warning: ";
			cout << CALCULATOR->message()->message() << endl;
			if(CALCULATOR->message()->message() == _("The calculation has been forcibly terminated. Please restart the application and report this as a bug.")) exit(0);
			b_ret = true;
		}
		if(!CALCULATOR->nextMessage()) break;
	}
	return b_ret;
}
void clear_errors() {
	if(!CALCULATOR->message()) return;
	while(true) {
		if(!CALCULATOR->nextMessage()) break;
	}
}
int successes = 0, imaginary = 0, s1 = 0, s2 = 0, f1 = 0;
void test_integration5(const MathStructure &mstruct, const Number &a, const Number &b) {
	cerr << "integrate(" << mstruct.print(CALCULATOR->messagePrintOptions()) << ", " << a << "," << b << ")" << endl;
	EvaluationOptions eo;
	eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	eo.assume_denominators_nonzero = true;
	eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	MathStructure mstruct2(mstruct);
	mstruct2.transform(CALCULATOR->f_integrate);
	mstruct2.addChild(a);
	mstruct2.addChild(b);
	mstruct2.addChild(CALCULATOR->v_x);
	mstruct2.addChild(m_zero);
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	CALCULATOR->calculate(&mstruct2, 10000, eo);
	cerr << "C" << endl;
	int i = has_not_a_comparison();
	if(i != 0) cout << i << "A: integrate(" << mstruct.print(CALCULATOR->messagePrintOptions()) << ", " << a << "," << b << ")" << endl;
	struct timespec ts2;
	clock_gettime(CLOCK_MONOTONIC, &ts2);
	if(ts2.tv_sec > ts.tv_sec + 3) cout << "SLOW: integrate(" << mstruct.print(CALCULATOR->messagePrintOptions()) << ", " << a << "," << b << ")" << endl;
	if(!mstruct2.isNumber()) {f1++; CALCULATOR->clearMessages(); return;}
	//if(!mstruct2.number().isReal()) {CALCULATOR->clearMessages(); imaginary++; return;}
	if(!mstruct2.number().isReal()) imaginary++;
	s1++;
	MathStructure mstruct3(mstruct);
	mstruct3.transform(CALCULATOR->f_integrate);
	mstruct3.addChild(a);
	mstruct3.addChild(b);
	mstruct3.addChild(CALCULATOR->v_x);
	mstruct3.addChild(m_one);
	CALCULATOR->calculate(&mstruct3, 10000, eo);
	cerr << "D" << endl;
	i = has_not_a_comparison();
	if(i != 0) cout << i << "B: integrate(" << mstruct.print(CALCULATOR->messagePrintOptions()) << ", " << a << "," << b << ")" << endl;
	//if(!mstruct3.isNumber()) {CALCULATOR->clearMessages(); return;}
	if(mstruct3.isNumber()) s2++;
	if(mstruct2.isNumber() && mstruct3.isNumber()) {
		if(!mstruct2.equals(mstruct3, true, true)) {
			PrintOptions po = CALCULATOR->messagePrintOptions();
			Number nr_i1, nr_i2, nr1, nr2;
			nr1 = mstruct2.number().realPart();
			nr2 = mstruct3.number().realPart();
			nr_i1 = mstruct2.number().imaginaryPart();
			nr_i2 = mstruct3.number().imaginaryPart();
			bool b_equal = true;
			string str1, str2;
			if(nr1.isNonZero() || nr2.isNonZero()) {
				if(nr2.precision() >= 0) po.min_decimals = nr2.precision(true);
				else po.min_decimals = 0;
				str1 = nr1.print(po);
				str2 = nr2.print(po);
				if(str1.length() != str2.length()) {
					PrintOptions po2 = po;
					po2.use_max_decimals = true;
					size_t p1 = str2.find(".");
					size_t p2 = str2.find("E");
					if(p2 == string::npos) p2 = str2.length();
					if(p1 == string::npos) {
						po2.max_decimals = 0;
					} else {
						po2.max_decimals = p2 - p1 - 1;
					}
					p1 = str1.find(".");
					p2 = str1.find("E");
					if(p2 == string::npos) p2 = str1.length();
					if(p1 == string::npos) {
						po2.max_decimals = 0;
					} else if(po2.max_decimals > (int) (p2 - p1 - 1)) {
						po2.max_decimals = p2 - p1 - 1;
					}
					po2.min_decimals = po2.max_decimals;
					str1 = nr1.print(po2);
					str2 = nr2.print(po2);
				}
				if(str1 != str2) b_equal = false;
			}
			if(nr_i1.isNonZero() || nr_i2.isNonZero()) {
				if(nr_i2.precision() >= 0) po.min_decimals = nr_i2.precision(true);
				else po.min_decimals = 0;
				string str_i1 = nr_i1.print(po);
				string str_i2 = nr_i2.print(po);
				if(str_i1.length() != str_i2.length()) {
					PrintOptions po2 = po;
					po2.use_max_decimals = true;
					size_t p1 = str_i2.find(".");
					size_t p2 = str_i2.find("E");
					if(p2 == string::npos) p2 = str_i2.length();
					if(p1 == string::npos) {
						po2.max_decimals = 0;
					} else {
						po2.max_decimals = p2 - p1 - 1;
					}
					p1 = str_i1.find(".");
					p2 = str_i1.find("E");
					if(p2 == string::npos) p2 = str_i1.length();
					if(p1 == string::npos) {
						po2.max_decimals = 0;
					} else if(po2.max_decimals > (int) (p2 - p1 - 1)) {
						po2.max_decimals = p2 - p1 - 1;
					}
					po2.min_decimals = po2.max_decimals;
					str_i1 = nr_i1.print(po2);
					str_i2 = nr_i2.print(po2);
				}
				if(str_i1 != str_i2) b_equal = false;
				if(str1.empty()) str1 = str_i1;
				else {str1 += " + "; str1 += str_i1; str1 += "i";}
				if(str2.empty()) str2 = str_i2;
				else {str2 += " + "; str2 += str_i2; str2 += "i";}
			}

			if(!b_equal) {
				po.min_decimals = 0;
				cout << "Integration test: integrate(" << mstruct.print(CALCULATOR->messagePrintOptions()) << ", " << a << "," << b << ")" << endl;
				display_errors();
				cout << str1 << endl;
				cout << str2 << endl;
				cout << "________________________________________________" << endl;
			} else {
				successes++;
				//if(!mstruct2.isNumber() || !mstruct2.number().isReal()) imaginary++;
			}
		} else {
			successes++;
			//if(!mstruct2.isNumber() || !mstruct2.number().isReal()) imaginary++;
		}
	}
	cerr << "E" << endl;
	CALCULATOR->clearMessages();
}
void test_integration6(const MathStructure &mstruct, const Number &a, const Number &b) {
	MathStructure x_var(CALCULATOR->v_x);
	cout << "Integration test: " << mstruct.print(CALCULATOR->messagePrintOptions()) << endl;
	MathStructure mstruct2(mstruct);
	EvaluationOptions eo;
	eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	eo.assume_denominators_nonzero = true;
	mstruct2.integrate(x_var, eo, true, -1, false, true, 4);
	if(mstruct2.containsFunction(CALCULATOR->f_integrate)) {clear_errors(); return;}
	mstruct2.differentiate(x_var, eo);
	mstruct2.eval(eo);
	MathStructure mstruct3(mstruct2);
	mstruct3.replace(x_var, a);
	mstruct3.eval(eo);
	display_errors();
	string str1 = mstruct3.print(CALCULATOR->messagePrintOptions());
	cout << str1 << endl;
	mstruct3 = mstruct;
	mstruct3.replace(x_var, a);
	mstruct3.eval(eo);
	display_errors();
	string str2 = mstruct3.print(CALCULATOR->messagePrintOptions());
	cout << str2 << endl;
	if(str1 != str2) cout << "!!!" << endl;
	mstruct3 = mstruct2;
	mstruct3.replace(x_var, b);
	mstruct3.eval(eo);
	display_errors();
	str1 = mstruct3.print(CALCULATOR->messagePrintOptions());
	cout << str1 << endl;
	mstruct3 = mstruct;
	mstruct3.replace(x_var, b);
	mstruct3.eval(eo);
	display_errors();
	str2 = mstruct3.print(CALCULATOR->messagePrintOptions());
	cout << str2 << endl;
	if(str1 != str2) cout << "!!!" << endl;
	cout << "________________________________________________" << endl;
}
void test_integration4(const MathStructure &mstruct) {
	//test_integration5(mstruct, Number(2, 1), Number(7, 3));
	test_integration6(mstruct, 3, -5);
}
void test_integration3(const MathStructure &mstruct, const MathStructure &mstruct_arg) {
	MathStructure mstruct2(mstruct);
	test_integration4(mstruct2);
	mstruct2 = mstruct;
	mstruct2 *= CALCULATOR->v_x;
	test_integration4(mstruct2);
	mstruct2.last() ^= nr_two;
	test_integration4(mstruct2);
	mstruct2.last()[1] = nr_minus_one;
	test_integration4(mstruct2);
	mstruct2 = mstruct;
	mstruct2 *= mstruct_arg;
	test_integration4(mstruct2);
	mstruct2 = mstruct;
	mstruct2 *= mstruct;
	test_integration4(mstruct2);
}
void test_integration2(const MathStructure &mstruct) {
	MathStructure mstruct2(mstruct);
	mstruct2.transform(CALCULATOR->f_ln);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 *= CALCULATOR->getRadUnit();
	mstruct2.transform(CALCULATOR->f_sin);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 *= CALCULATOR->getRadUnit();
	mstruct2.transform(CALCULATOR->f_cos);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 *= CALCULATOR->getRadUnit();
	mstruct2.transform(CALCULATOR->f_tan);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_asin);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_acos);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_atan);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_sinh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_cosh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_tanh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_asinh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_acosh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2.transform(CALCULATOR->f_atanh);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= nr_two;
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= nr_minus_one;
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= Number(-2, 1);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= Number(3, 1);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= Number(-3, 1);
	test_integration3(mstruct2, mstruct);
	mstruct2 = mstruct;
	mstruct2 ^= Number(1, 3);
	test_integration3(mstruct2, mstruct);
}
void test_integration() {
	MathStructure mstruct;
	CALCULATOR->parse(&mstruct, "4x+5");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "-2x+7");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "4.7x-5.2");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "-4.3x-5");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "4x");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "-2.3x");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "x+6");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "x-7");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "x");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "x^2");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "2x^2+5");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "-2x^2-5");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "sqrt(x)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "sqrt(3x+3)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "5*sqrt(3x)-2");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "cbrt(3x+3)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "(3x+3)^(1/3)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "cbrt(x)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "x^(1/3)");
	test_integration2(mstruct);
	CALCULATOR->parse(&mstruct, "5^x");
	test_integration2(mstruct);
}

void test_intervals(bool use_interval) {

	CALCULATOR->useIntervalArithmetic(use_interval);
	PrintOptions po;
	//po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;

	vector<Number> nrs;

	nrs.push_back(nr_plus_inf);
	nrs.push_back(nr_minus_inf);
	nrs.push_back(nr_zero);
	nrs.push_back(nr_half);
	nrs.push_back(nr_minus_half);
	nrs.push_back(nr_one);
	nrs.push_back(nr_minus_one);
	nrs.push_back(nr_two);
	Number nr;

#define INCLUDES_INFINITY(x) (x.includesInfinity())
#define IS_INTERVAL(x) (x.isInterval(false))
	nr.setInterval(nr_minus_half, nr_half); nrs.push_back(nr);
	nr.setImaginaryPart(nr_one); nrs.push_back(nr); nr.setImaginaryPart(nr_minus_half); nrs.push_back(nr);
	nr.setImaginaryPart(nr_plus_inf); nrs.push_back(nr);
	nr.setInterval(nr_minus_one, nr_half); nrs.push_back(nr);
	nr.setInterval(nr_minus_half, nr_one); nrs.push_back(nr);
	nr.setInterval(nr_minus_one, nr_one); nrs.push_back(nr);
	nr.setInterval(Number(-2, 1), nr_two); nrs.push_back(nr);
	nr.setInterval(nr_zero, nr_half); nrs.push_back(nr);
	nr.setImaginaryPart(nrs[nrs.size() - 2]); nrs.push_back(nr); nr.setImaginaryPart(nr_one); nrs.push_back(nr); nr.setImaginaryPart(nr_minus_half); nrs.push_back(nr);
	nr.setImaginaryPart(nr_plus_inf); nrs.push_back(nr);
	nr.setInterval(nr_minus_half, nr_zero); nrs.push_back(nr);
	nr.setInterval(nr_zero, nr_two); nrs.push_back(nr);
	nr.setInterval(nr_half, nr_one); nrs.push_back(nr);
	nr.setImaginaryPart(nrs[nrs.size() - 2]); nrs.push_back(nr);
	nr.setInterval(nr_one, nr_two); nrs.push_back(nr);
	nr.setInterval(nr_two, nr_three); nrs.push_back(nr);
	nr.setInterval(nr_minus_one, nr_minus_half); nrs.push_back(nr);
	nr.setImaginaryPart(nrs[nrs.size() - 2]); nrs.push_back(nr);
	nr.setInterval(nr_minus_one, Number(-2, 1)); nrs.push_back(nr);
	nr.setInterval(Number(-2, 1), Number(-3, 1)); nrs.push_back(nr);
	nr.setInterval(nr_minus_inf, nr_minus_one); nrs.push_back(nr);
	nr.setInterval(nr_minus_inf, nr_one); nrs.push_back(nr);
	nr.setInterval(nr_minus_inf, nr_plus_inf); nrs.push_back(nr);
	nr.setInterval(nr_plus_inf, nr_minus_one); nrs.push_back(nr);
	nr.setInterval(nr_plus_inf, nr_one); nrs.push_back(nr);
	nr.setImaginaryPart(nrs[nrs.size() - 2]); nrs.push_back(nr); nr.setImaginaryPart(nr_one); nrs.push_back(nr); nr.setImaginaryPart(nr_minus_inf); nrs.push_back(nr);
	nr.setFloat(0.5); nrs.push_back(nr);
	nr.setFloat(-0.5); nrs.push_back(nr);
	nr.setImaginaryPart(nrs[nrs.size() - 2]); nr.setImaginaryPart(nr_one); nrs.push_back(nr);
	nr.setFloat(1.5); nrs.push_back(nr);
	nr.setFloat(-1.5); nrs.push_back(nr);

	Number nrsum1, nrsum2, nrsum3;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " * " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.multiply(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
		if(!INCLUDES_INFINITY(nrs[i])) {
			nrsum1 *= nrs[i];
			if(!IS_INTERVAL(nrs[i])) nrsum2 *= nrs[i];
		}
	}
	cout << "SUM1:" << nrsum1.print(po) << endl;
	cout << "SUM2:" << nrsum2.print(po) << endl;
	cout << "SUM3:" << nrsum3.print(po) << endl;
	cout << "________________________________________________" << endl;

	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " + " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.add(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
		if(!INCLUDES_INFINITY(nrs[i])) {
			nrsum1 += nrs[i];
			if(!IS_INTERVAL(nrs[i])) nrsum2 += nrs[i];
		}
	}
	cout << "SUM1:" << nrsum1.print(po) << endl;
	cout << "SUM2:" << nrsum2.print(po) << endl;
	cout << "SUM3:" << nrsum3.print(po) << endl;
	cout << "________________________________________________" << endl;

	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " - " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.subtract(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
		if(!INCLUDES_INFINITY(nrs[i])) {
			nrsum1 -= nrs[i];
			if(!IS_INTERVAL(nrs[i])) nrsum2 -= nrs[i];
		}
	}
	cout << "SUM1:" << nrsum1.print(po) << endl;
	cout << "SUM2:" << nrsum2.print(po) << endl;
	cout << "SUM3:" << nrsum3.print(po) << endl;
	cout << "________________________________________________" << endl;

	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " / " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.divide(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
		if(!INCLUDES_INFINITY(nrs[i])) {
			nrsum1 /= nrs[i];
			if(!IS_INTERVAL(nrs[i])) nrsum2 /= nrs[i];
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;

	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " ^ " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.raise(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}

	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;

	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " log " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.log(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 0; i2 < nrs.size(); i2++) {
			cout << nrs[i].print(po) << " atan2 " << nrs[i2].print(po) << " = ";
			nr.set(nrs[i]);
			if(nr.atan2(nrs[i2])) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(size_t i2 = 2; i2 < 10; i2++) {
			cout << nrs[i].print(po) << " root " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.root(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(long int i2 = -3; i2 <= 3; i2++) {
			cout << nrs[i].print(po) << " + " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.add(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(long int i2 = -3; i2 <= 3; i2++) {
			cout << nrs[i].print(po) << " - " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.subtract(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(long int i2 = -3; i2 <= 3; i2++) {
			cout << nrs[i].print(po) << " * " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.multiply(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(long int i2 = -3; i2 <= 3; i2++) {
			cout << nrs[i].print(po) << " / " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.divide(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		for(long int i2 = -3; i2 <= 3; i2++) {
			cout << nrs[i].print(po) << " + " << i2 << " = ";
			nr.set(nrs[i]);
			if(nr.add(i2)) cout << nr.print(po) << endl;
			else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
		}
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "inv(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.recip()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "neg(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.negate()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "abs(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.abs()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "sq(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.square()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "sqrt(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.sqrt()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "cbrt(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.cbrt()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "sin(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.sin()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "asin(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.asin()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "sinh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.sinh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "asinh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.asinh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "cos(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.cos()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "acos(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.acos()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "cosh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.cosh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "acosh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.acosh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "tan(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.tan()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "atan(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.atan()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "tanh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.tanh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "atanh(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.atanh()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "ln(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.ln()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "gamma(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.gamma()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "digamma(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.digamma()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "besselj(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.besselj(1)) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "bessely(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.bessely(1)) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "erf(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.erf()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "erfc(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.erfc()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}
	cout << "SUM:" << nrsum3.print(po) << endl; cout << "________________________________________________" << endl;
	for(size_t i = 0; i < nrs.size(); i++) {
		cout << "arg(" << nrs[i].print(po) << ") = ";
		nr.set(nrs[i]);
		if(nr.arg()) cout << nr.print(po) << endl;
		else cout << "FAILED" << endl;
			if(!INCLUDES_INFINITY(nr)) nrsum3 += nr;
	}

	/*Number nr1, nr2, nr3, nr4, nr5;
	nr1.setInterval(Number(1, 1), Number(2, 1));
	nr2.setInterval(Number(3, 1), Number(4, 1));
	nr3.setInterval(Number(-2, 1), Number(1, 1));
	nr4.setInterval(Number(-1, 1), Number(-2, 1));
	nr5.setInterval(Number(-3, 1), Number(-4, 1));

	Number n1(2, 1), n2(4, 1), n3(-2, 1), n4(1, 2);

	Number nrm;
	cout << "square" << endl;
	nrm = nr1; nrm.square();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.square();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.square();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.square();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.square();
	cout << "5: " << nrm.print() << endl;
	cout << "abs" << endl;
	nrm = nr1; nrm.abs();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.abs();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.abs();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.abs();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.abs();
	cout << "5: " << nrm.print() << endl;
	cout << "sin" << endl;
	nrm = nr1; nrm.sin();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.sin();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.sin();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.sin();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.sin();
	cout << "5: " << nrm.print() << endl;
	cout << "asin" << endl;
	nrm = nr1; nrm.asin();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.asin();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.asin();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.asin();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.asin();
	cout << "5: " << nrm.print() << endl;
	cout << "asinh" << endl;
	nrm = nr1; nrm.asinh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.asinh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.asinh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.asinh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.asinh();
	cout << "5: " << nrm.print() << endl;
	cout << "sinh" << endl;
	nrm = nr1; nrm.sinh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.sinh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.sinh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.sinh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.sinh();
	cout << "5: " << nrm.print() << endl;
	cout << "cos" << endl;
	nrm = nr1; nrm.cos();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.cos();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.cos();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.cos();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.cos();
	cout << "5: " << nrm.print() << endl;
	cout << "acos" << endl;
	nrm = nr1; nrm.acos();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.acos();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.acos();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.acos();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.acos();
	cout << "5: " << nrm.print() << endl;
	cout << "acosh" << endl;
	nrm = nr1; nrm.acosh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.acosh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.acosh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.acosh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.acosh();
	cout << "5: " << nrm.print() << endl;
	cout << "cosh" << endl;
	nrm = nr1; nrm.cosh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.cosh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.cosh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.cosh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.cosh();
	cout << "5: " << nrm.print() << endl;
	cout << "tan" << endl;
	nrm = nr1; nrm.tan();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.tan();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.tan();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.tan();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.tan();
	cout << "5: " << nrm.print() << endl;
	cout << "atan" << endl;
	nrm = nr1; nrm.atan();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.atan();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.atan();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.atan();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.atan();
	cout << "5: " << nrm.print() << endl;
	cout << "atanh" << endl;
	nrm = nr1; nrm.atanh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.atanh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.atanh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.atanh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.atanh();
	cout << "5: " << nrm.print() << endl;
	cout << "tanh" << endl;
	nrm = nr1; nrm.tanh();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.tanh();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.tanh();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.tanh();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.tanh();
	cout << "5: " << nrm.print() << endl;
	cout << "gamma" << endl;
	nrm = nr1; nrm.gamma();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.gamma();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.gamma();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.gamma();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.gamma();
	cout << "5: " << nrm.print() << endl;
	cout << "digamma" << endl;
	nrm = nr1; nrm.digamma();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.digamma();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.digamma();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.digamma();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.digamma();
	cout << "5: " << nrm.print() << endl;
	cout << "erf" << endl;
	nrm = nr1; nrm.erf();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.erf();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.erf();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.erf();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.erf();
	cout << "5: " << nrm.print() << endl;
	cout << "erfc" << endl;
	nrm = nr1; nrm.erfc();
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.erfc();
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.erfc();
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.erfc();
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.erfc();
	cout << "5: " << nrm.print() << endl;
	cout << "misc" << endl;
	nrm = nr1; nrm.multiply(2);
	cout << "1: " << nrm.print() << endl;
	nrm = nr2; nrm.multiply(-2);
	cout << "2: " << nrm.print() << endl;
	nrm = nr3; nrm.subtract(2);
	cout << "3: " << nrm.print() << endl;
	nrm = nr4; nrm.divide(-2);
	cout << "4: " << nrm.print() << endl;
	nrm = nr5; nrm.add(-2);
	cout << "5: " << nrm.print() << endl;*/

}

string rnd_expression(int allow_unknowns, bool allow_functions, int length_factor1 = 10, int length_factor2 = 5, bool allow_units = false, bool allow_variables = false, bool allow_interval = false, bool allow_complex = true, bool only_integers = false, int num_ratio = 2, bool num_ratio_den = false);


string rnd_unit() {
	int r = rand() % CALCULATOR->units.size();
	Unit *u = CALCULATOR->units[r];
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) return ((CompositeUnit*) u)->print(false, true);
	string str;
	if(rand() % 3 == 0) {
		r = rand() % CALCULATOR->prefixes.size();
		str += CALCULATOR->prefixes[r]->name();
	}
	str += u->referenceName();
	if(rand() % 3 == 0) {
		str += "^";
		str += ('2' + rand() % 2);
	}
	return str;
}

string rnd_var() {
	/*if(rand() % 2 == 1) return "e";
	else return "pi";*/
	while(true) {
		int r = rand() % CALCULATOR->variables.size();
		if(CALCULATOR->variables[r]->isKnown() && CALCULATOR->variables[r]->referenceName() != "uptime") return CALCULATOR->variables[r]->name();
	}
	return "";
}


string rnd_number(bool use_par = true, bool only_integers = false, bool only_positive = false, bool allow_complex = true, bool allow_interval = false) {
	string str;
	bool par = false;
	bool dot = only_integers;
	bool started = false;
	if(!only_positive && rand() % 4 == 0) {str += '-'; par = true;}
	/*str += '0' + rand() % 10;
	if(!only_integers && str[str.length() - 1] != '0' && rand() % 4 == 0) {
		str += '/';
		str += '1' + rand() % 9;
		par = true;
	}*/
	while(true && str.length() < 20) {
		int r = rand();
		if(!started) r = r % (only_positive ? 9 + 1 : 10 + 1);
		else if(str.back() == '.') r = r % 10;
		else r = r % ((dot ? 19 : 20) + str.length() * 10);
		if(r > (dot ? 9 : 15)) {
			if(started) break;
		} else {
			if((r >= 10 && r <= 15) || (!dot && !started && r == 0)) {if(!started) str += '0'; str += '.'; dot = true;}
			else str += char('0' + r);
			started = true;
		}
	}
	if(allow_complex && !only_integers && rand() % 10 == 0) {str += 'i'; par = true;}
	else if(allow_interval && rand() % 2 == 0 && str.find(".") == string::npos) {str += "+/-4E-8"; /*str += rnd_number(false, true, true, false, false);*/}
	if(str.find_first_not_of("-0.i") == string::npos) return rnd_number(use_par, only_integers, only_positive, allow_complex, allow_interval);
	if(par && use_par) {str += ')'; str.insert(0, "(");}
	return str;
}

string rnd_item(int &par, bool allow_function = true, int allow_unknown = 1, int allow_unit = false, int allow_variable = false, bool allow_interval = false, bool allow_complex = true, bool only_integers = false, int num_ratio = 2, bool num_ratio_den = false) {
	int r = rand() % num_ratio + 1;
	string str;
	if((num_ratio_den ? (r != 1) : (r == 1)) || (!allow_unknown && !allow_function && !allow_unit && !allow_variable)) {
		str = rnd_number(true, only_integers, false, allow_complex, allow_interval);
	} else {
		if(allow_unit && (rand() % (2 + allow_function + allow_unknown + allow_variable)) == 0) {
			str = rnd_unit();
		} else if(allow_variable && (rand() % (2 + allow_function + allow_unknown)) == 0) {
			if(!allow_function && !allow_unknown && !allow_variable && rand() % 2 == 0) str = rnd_number(true, only_integers, false, allow_complex, allow_interval);
			else str = rnd_var();
		} else {
#define LAST_FUNC 23
			if(!allow_unknown) {
				if(allow_function) r = rand() % (LAST_FUNC - 4)  + 4;
				else r = rand() % 2 + 4;
			} else {
				int au2 = 3 - allow_unknown % 3;
				r = (rand() % ((allow_function ? LAST_FUNC - 1 + allow_unknown : 5) - au2)) + 4 - allow_unknown;
				if(r < 4 - allow_unknown % 3) {
					if(r < 0) r = -r;
					if(allow_unknown % 3 == 1) r = 3;
					else if(allow_unknown % 3 == 2) r = 2 + r % 2;
					else r = 1 + r % 3;
				}
			}
			switch(r) {
				case 1: {str = "z"; break;}
				case 2: {str = "y"; break;}
				case 3: {str = "x"; break;}
				case 4: {str = "pi"; break;}
				case 5: {str = "e"; break;}
				case 6: {str = "root(";
					str += rnd_expression(allow_unknown, allow_function, 6, 3, allow_unit, allow_variable, allow_interval, allow_complex, only_integers, num_ratio, num_ratio_den);
					str += ',';
					str += rnd_number(true, true, true, false, false);
					str += ')';
					return str;
				}
				case 7: {str = "log(";
					str += rnd_expression(allow_unknown, allow_function, 6, 3, allow_unit, allow_variable, allow_interval, allow_complex, only_integers, num_ratio, num_ratio_den);
					str += ',';
					str += rnd_number(true, true, true, false, false);
					str += ')';
					return str;
				}
				case 8: {str = "ln("; break;}
				case 9: {str = "abs("; break;}
				case 10: {str = "sin("; break;}
				case 11: {str = "cos("; break;}
				case 12: {str = "tan("; break;}
				case 13: {str = "sinh("; break;}
				case 14: {str = "cosh("; break;}
				case 15: {str = "tanh("; break;}
				case 16: {str = "asin("; break;}
				case 17: {str = "acos("; break;}
				case 18: {str = "atan("; break;}
				case 19: {str = "asinh("; break;}
				case 20: {str = "acosh("; break;}
				case 21: {str = "atanh("; break;}
				case 22: {str = "sqrt("; break;}
				case 23: {str = "cbrt("; break;}
				case 24: {str = "erf("; break;}
				case 25: {str = "erfc("; break;}
				case 26: {str = "Si("; break;}
				case 27: {str = "Shi("; break;}
				case 28: {str = "Ci("; break;}
				case 29: {str = "Chi("; break;}
				case 30: {str = "sinc("; break;}
				case 31: {str = "li("; break;}
				case 32: {str = "lambertw("; break;}
				case 33: {str = "digamma("; break;}
				case 34: {str = "im("; break;}
				case 35: {str = "re("; break;}
				case 36: {str = "airy("; break;}
			}
			if(r > 5) {
				if(allow_unknown && rand() % 2 == 1) {
					str += "x)";
					if(rand() % 3 == 1) str += "^2";
				} else {
					str += rnd_item(par, true, allow_unknown, allow_unit, allow_variable, allow_interval, allow_complex, only_integers, num_ratio, num_ratio_den);
					par++;
				}
			}
		}
	}
	r = rand() % (5 * (par + 1)) + 1;
	if(r == 1) {
		str.insert(0, "(");
		par++;
	}
	return str;
}
string rnd_operator(int &par, bool allow_pow = true) {
	int r = rand() % (par ? 10 : 5) + 1;
	string str;
	if(par > 0 && r > 5) {
		par--;
		str = ")";
		r -= 5;
	}
	if(r == 5 && !allow_pow) r = rand() % 4 + 1;
	switch(r) {
		case 1: return str + "+";
		case 2: return str + "-";
		case 3: return str + "*";
		case 4: return str + "/";
		case 5: {
			switch(rand() % 8 + 1) {
				case 1: return str + "^";
				case 2: return str + "^2" + rnd_operator(par, false);
				case 3: return str + "^3" + rnd_operator(par, false);
				case 4: return str + "^-1" + rnd_operator(par, false);
				case 5: return str + "^(1/2)" + rnd_operator(par, false);
				case 6: return str + "^(1/3)" + rnd_operator(par, false);
				default: {return str + "^(" + rnd_number(true, true) + "/" + rnd_number(true, true) + ")" + rnd_operator(par, false);}
			}
		}
	}
	return "";
}

string rnd_expression(int allow_unknowns, bool allow_functions, int length_factor1, int length_factor2, bool allow_unit, bool allow_variable, bool allow_interval, bool allow_complex, bool only_integers, int num_ratio, bool num_ratio_den) {
	int par = 0;
	string str;
	while(str.empty() || rand() % ((length_factor1 - (int) str.length() / length_factor2 < 2) ? 2 : (length_factor1 - (int) str.length() / length_factor2)) != 0) {
		str += rnd_item(par, allow_functions, allow_unknowns, allow_unit, allow_variable, allow_interval, allow_complex, only_integers);
		str += rnd_operator(par);
	}
	if(str.back() != ')') str += rnd_item(par, false, allow_unknowns, allow_unit, allow_variable, allow_interval, allow_complex, only_integers);
	while(par > 0) {
		str += ")";
		par--;
	}
	return str;
}

KnownVariable *v;

bool contains_division_by_zero(const MathStructure &mstruct) {
	if(mstruct.isPower() && mstruct[0].isNumber() && !mstruct[0].number().isNonZero() && mstruct[1].representsNegative()) return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_division_by_zero(mstruct[i])) return true;
	}
	return false;
}

extern bool testComplex(Number *this_nr, Number *i_nr);

int rt1 = 0, rt2 = 0, rt3 = 0, rt4 = 0, rt5 = 0, rt6 = 0, rt7 = 0, rt8 = 0, rt9 = 0;
void rnd_test(EvaluationOptions eo, int allow_unknowns, bool allow_functions, bool test_interval = true, bool test_equation = true, bool allow_unit = false, bool allow_variable = false, bool allow_interval = false) {
	bool b_iv = CALCULATOR->usesIntervalArithmetic();
	IntervalCalculation ic = eo.interval_calculation;
	cerr << "A0" << endl;
	string str = rnd_expression(allow_unknowns, allow_functions, 6, 4, allow_unit, allow_variable, allow_interval);
	cerr << "A2:" << str << endl;
	PrintOptions po; po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; po.use_max_decimals = true; po.max_decimals = 2; po.min_exp = 1;
	MathStructure mp, m1, m2, m3, m4;
	CALCULATOR->parse(&mp, str, eo.parse_options);
	eo.approximation = APPROXIMATION_APPROXIMATE;
	m1 = mp;
	m3 = mp;
	m4 = mp;
	cerr << "A3" << endl;
	cerr << mp << endl;
	eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	CALCULATOR->calculate(&m1, 5000, eo);
	if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	cerr << "A3b" << endl;
	/*eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
	CALCULATOR->calculate(&m3, 5000, eo);
	if(m3.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1b" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	cerr << "A3c" << endl;
	eo.interval_calculation = INTERVAL_CALCULATION_NONE;
	CALCULATOR->calculate(&m4, 5000, eo);
	if(m4.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1c" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	cerr << "A3d" << endl;*/
	eo.interval_calculation = ic;
	eo.approximation = APPROXIMATION_EXACT;
	m2 = mp;
	CALCULATOR->calculate(&m2, 5000, eo);
	if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED2" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	cerr << "A3e: " << m2 << endl;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	CALCULATOR->calculate(&m2, 5000, eo);
	cerr << "A3f" << endl;
	if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED3" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	if(m1.isNumber()) {
		rt1++;
		if(m2.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m2))) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1: " << m1.print(po) << ":" << m2.print(po) << endl;
			}
		}
		/*if(m3.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m3))) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1b: " << m1.print(po) << ":" << m3.print(po) << endl;
			}
		}
		if(m4.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m4)) && m1.print(po) != m4.print(po)) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1c: " << m1.print(po) << ":" << m4.print(po) << endl;
			}
		}*/
	}
	cerr << "A" << endl;
	if(b_iv != CALCULATOR->usesIntervalArithmetic()) {
		cout << "INTERVAL ARITHMETIC CHANGED1: " << str << " => " << mp << endl;
		CALCULATOR->useIntervalArithmetic(b_iv);
	}
	if(mp.contains(CALCULATOR->v_x)) {
		rt3++;
		Number nr(rnd_number(false, false, false, true, allow_interval));
		if(nr.hasImaginaryPart() && rand() % 2 == 0) nr += Number(rnd_number(false, false, false, false, false));
		m1 = mp;
		m1.replace(CALCULATOR->v_x, nr);
		m3 = m1;
		cerr << "A2:" << m1 << endl;
		eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		CALCULATOR->calculate(&m1, 5000, eo);
		if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED4: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		/*eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		CALCULATOR->calculate(&m3, 5000, eo);
		if(m3.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED4b: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		eo.interval_calculation = ic;*/
		m2 = mp;
		CALCULATOR->v_x->setAssumptions(nr);
		eo.approximation = APPROXIMATION_EXACT;
		CALCULATOR->calculate(&m2, 5000, eo);
		CALCULATOR->v_x->setAssumptions(NULL);
		if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED5: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		m2.replace(CALCULATOR->v_x, nr);
		MathStructure m4 = m2;
		eo.approximation = APPROXIMATION_APPROXIMATE;
		CALCULATOR->calculate(&m4, 5000, eo);
		if(m4.isAborted()) {cout << str << " => " << m4 << endl; cout << "ABORTED6: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		rt4++;
		/*if(COMPARISON_IS_NOT_EQUAL(m1.compare(m3))) {
			rt5++;
			cout << str << " => " << mp << ":" << nr << endl;
			cout << "UNEQUAL2a: " << m1.print(po) << ":" << m3.print(po) << endl;
		}*/
		if(m4.isNumber()) testComplex(&m4.number(), m4.number().internalImaginary());
		if(m1.isNumber()) testComplex(&m1.number(), m1.number().internalImaginary());
		if(COMPARISON_IS_NOT_EQUAL(m1.compare(m4)) && !m1.containsInfinity() && !m4.containsInfinity() && !contains_division_by_zero(m1) && !contains_division_by_zero(m4)) {
			rt5++;
			cout << str << " => " << mp << ":" << nr << endl;
			cout << "UNEQUAL2b: " << m1.print(po) << ":" << m4.print(po) << endl;
		}
		if(b_iv != CALCULATOR->usesIntervalArithmetic()) {
			cout << "INTERVAL ARITHMETIC CHANGED3: " << str << " => " << mp << endl;
			CALCULATOR->useIntervalArithmetic(b_iv);
		}
		if(test_interval) {
			m1 = mp;
			if(nr.hasImaginaryPart()) nr = rand() % 100 - 50;
			nr.setPrecision(5);
			Number nr_iv = nr;
			nr_iv.precisionToInterval();
			cerr << "I1: " << nr_iv << endl;
			v->set(nr_iv);
			m1.replace(CALCULATOR->v_x, v);
			m2 = m1;
			eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
			CALCULATOR->calculate(&m1, 5000, eo);
			if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED9"; CALCULATOR->useIntervalArithmetic(b_iv); return;}
			eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
			v->set(nr);
			cerr << "I2" << endl;
			CALCULATOR->calculate(&m2, 5000, eo);
			eo.interval_calculation = ic;
			if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED10"; CALCULATOR->useIntervalArithmetic(b_iv); return;}
			if(m1.isNumber() && m2.isNumber() && m1.number().precision(true) > 2) {
				rt8++;
				PrintOptions po;
				po.min_exp = 1;
				po.max_decimals = 2;

				po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				if(m1 != m2) {
					string si1 = m1.number().imaginaryPart().print(po);
					string si2 = m2.number().imaginaryPart().print(po);
					string sr1 = m1.number().realPart().print(po);
					string sr2 = m2.number().realPart().print(po);
					if(si1 != si2 && si1 == "0") {
						nr = m2.number().imaginaryPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							si2 = "0";
						}
					}
					if(sr1 != sr2 && sr1 == "0") {
						nr = m2.number().realPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							sr2 = "0";
						}
					}
					if(si1 != si2 && si2 == "0") {
						nr = m1.number().imaginaryPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							si1 = "0";
						}
					}
					if(sr1 != sr2 && sr2 == "0") {
						nr = m1.number().realPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							sr1 = "0";
						}
					}
					if((sr1 != sr2 && sr1.length() < 10) || (si1 != si2 && si1.length() < 10)) {
						po.max_decimals = 1;
						if(sr1 != sr2) sr2 = m2.number().realPart().print(po);
						if(si1 != si2) si2 = m2.number().imaginaryPart().print(po);
						if((sr1 != sr2 && sr1.length() < 10) || (si1 != si2 && si1.length() < 10)) {
							rt9++;
							cout << str << " => " << mp << ":" << nr_iv << endl;
							cout << "UNEQUAL4: " << m1.print(po) << ":" << m2.print(po) << endl;
						}
					}
				}
			}
		}

		if(test_equation) {
			m1 = mp;
			cerr << "B" << endl;
			m1.transform(COMPARISON_EQUALS, m_zero);
			//eo.approximation = APPROXIMATION_EXACT;
			CALCULATOR->calculate(&m1, 5000, eo);
			if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED7"; CALCULATOR->useIntervalArithmetic(b_iv); return;}
			m1.replace(CALCULATOR->v_n, 3);
			eo.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			if(m1.isComparison() && m1.comparisonType() == COMPARISON_EQUALS && m1[0] == CALCULATOR->v_x) {
				m2 = mp;
				m2.replace(CALCULATOR->v_x, m1[1]);
				eo.approximation = APPROXIMATION_APPROXIMATE;
				CALCULATOR->calculate(&m2, 5000, eo);
				if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED8: " << m1[1] << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
				if(m2.isNumber()) {
					rt6++;
					if(m2.number().isNonZero()) {
						nr = m2.number();
						nr.abs();
						nr.log(10);
						if(nr > -10) {
							rt7++;
							cout << str << " => " << mp << endl;
							cout << "UNEQUAL3: " << m1.print(po) << ":" << m2.print(po) << endl;
						}
					}
				}
			} else if(!m1.isComparison()) {
				bool  b = true;
				if(m1.type() == STRUCT_LOGICAL_AND) {
					for(size_t i = 0; i < m1.size(); i++) {
						if(m1[i].isComparison() && m1[i].comparisonType() != COMPARISON_EQUALS) {
							eo.approximation = APPROXIMATION_APPROXIMATE;
							CALCULATOR->calculate(&m1[i], 5000, eo);
							if(m1[i].isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED9: " << m1[i] << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
							if(m1[i].isZero()) {
								b = false;
								break;
							}
						}
					}
				}
				if(b) {
					for(size_t i = 0; i < m1.size(); i++) {
						if(m1[i].isComparison() && m1[i].comparisonType() == COMPARISON_EQUALS && m1[i][0] == CALCULATOR->v_x) {
							m2 = mp;
							m2.replace(CALCULATOR->v_x, m1[i][1]);
							eo.approximation = APPROXIMATION_APPROXIMATE;
							CALCULATOR->calculate(&m2, 5000, eo);
							if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED8: " << m1[i][1] << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
							if(m2.isNumber()) {
								rt6++;
								if(m2.number().isNonZero()) {
									nr = m2.number();
									nr.abs();
									nr.log(10);
									if(nr > -10) {
										rt7++;
										cout << str << " => " << mp << endl;
										cout << "UNEQUAL3: " << m1[i].print(po) << ":" << m2.print(po) << endl;
									}
								}
							}
						} else if(!m1[i].isComparison()) {
							b = true;
							if(m1[i].type() == STRUCT_LOGICAL_AND) {
								for(size_t i2 = 0; i2 < m1[i].size(); i2++) {
									if(m1[i][i2].isComparison() && m1[i][i2].comparisonType() != COMPARISON_EQUALS) {
										eo.approximation = APPROXIMATION_APPROXIMATE;
										CALCULATOR->calculate(&m1[i][i2], 5000, eo);
										if(m1[i][i2].isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED9: " << m1[i][i2] << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
										if(m1[i][i2].isZero()) {
											b = false;
											break;
										}
									}
								}
							}
							if(b) {
								for(size_t i2 = 0; i2 < m1[i].size(); i2++) {
									if(m1[i][i2].isComparison() && m1[i][i2].comparisonType() == COMPARISON_EQUALS && m1[i][i2][0] == CALCULATOR->v_x) {
										m2 = mp;
										m2.replace(CALCULATOR->v_x, m1[i][i2][1]);
										eo.approximation = APPROXIMATION_APPROXIMATE;
										CALCULATOR->calculate(&m2, 5000, eo);
										if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED8: " << m1[i][i2][1] << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
										if(m2.isNumber()) {
											rt6++;
											if(m2.number().isNonZero()) {
												nr = m2.number();
												nr.abs();
												nr.log(10);
												if(nr > -10) {
													rt7++;
													cout << str << " => " << mp << endl;
													cout << "UNEQUAL3: " << m1[i][i2].print(po) << ":" << m2.print(po) << endl;
												}
											}
										}
									}
								}
							}
						}
					}
				}
				if(b_iv != CALCULATOR->usesIntervalArithmetic()) {
					cout << "INTERVAL ARITHMETIC CHANGED5: " << str << " => " << mp << endl;
					CALCULATOR->useIntervalArithmetic(b_iv);
				}
			}
		}
	}

	string str2 = rnd_expression(allow_unknowns, allow_functions, 5, 4, allow_unit, allow_variable, allow_interval);
	str.insert(0, "(");
	str += ") / (";
	str += str2;
	str += ")";
	MathStructure mp2;
	CALCULATOR->parse(&mp2, str2, eo.parse_options);
	mp /= mp2;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	m1 = mp;
	m3 = m1;
	m4 = m1;
	cerr << "DEN:" << str2 << " => " << mp << endl;

	eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	CALCULATOR->calculate(&m1, 5000, eo);
	if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	/*eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
	CALCULATOR->calculate(&m3, 5000, eo);
	if(m3.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1b" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	eo.interval_calculation = INTERVAL_CALCULATION_NONE;
	CALCULATOR->calculate(&m4, 5000, eo);
	if(m4.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED1c" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	cerr << "A3d" << endl;*/
	eo.interval_calculation = ic;
	eo.approximation = APPROXIMATION_EXACT;
	m2 = mp;
	CALCULATOR->calculate(&m2, 5000, eo);
	if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED2" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	eo.approximation = APPROXIMATION_APPROXIMATE;
	CALCULATOR->calculate(&m2, 5000, eo);
	if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED3" << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
	if(m1.isNumber()) {
		rt1++;
		if(m2.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m2))) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1: " << m1.print(po) << ":" << m2.print(po) << endl;
			}
		}
		/*if(m3.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m3))) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1b: " << m1.print(po) << ":" << m3.print(po) << endl;
			}
		}
		if(m4.isNumber()) {
			if(COMPARISON_IS_NOT_EQUAL(m1.compare(m4)) && m1.print(po) != m4.print(po)) {
				rt2++;
				cout << str << " => " << mp << endl;
				cout << "UNEQUAL1c: " << m1.print(po) << ":" << m4.print(po) << endl;
			}
		}*/
	}
	if(b_iv != CALCULATOR->usesIntervalArithmetic()) {
		cout << "INTERVAL ARITHMETIC CHANGED12 " << str << " => " << mp << endl;
		CALCULATOR->useIntervalArithmetic(b_iv);
	}
	cerr << "C" << endl;
	if(mp.contains(CALCULATOR->v_x)) {
		rt3++;
		Number nr(rnd_number(false, false, false, true, allow_interval));
		if(nr.hasImaginaryPart() && rand() % 2 == 0) nr += Number(rnd_number(false, false, false, false, false));
		m1 = mp;
		m1.replace(CALCULATOR->v_x, nr);
		eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		m3 = m1;
		cerr << "A2:" << m1 << endl;
		CALCULATOR->calculate(&m1, 5000, eo);
		if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED4: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		/*eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		CALCULATOR->calculate(&m3, 5000, eo);
		if(m3.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED4b: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}*/
		eo.interval_calculation = ic;
		m2 = mp;
		eo.approximation = APPROXIMATION_EXACT;
		CALCULATOR->v_x->setAssumptions(nr);
		CALCULATOR->calculate(&m2, 5000, eo);
		CALCULATOR->v_x->setAssumptions(NULL);
		if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED5: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		m2.replace(CALCULATOR->v_x, nr);
		MathStructure m4 = m2;
		eo.approximation = APPROXIMATION_APPROXIMATE;
		CALCULATOR->calculate(&m4, 5000, eo);
		if(m4.isAborted()) {cout << str << " => " << m4 << endl; cout << "ABORTED6: " << nr << endl; CALCULATOR->useIntervalArithmetic(b_iv); return;}
		rt4++;
		/*if(COMPARISON_IS_NOT_EQUAL(m1.compare(m3))) {
			rt5++;
			cout << str << " => " << mp << ":" << nr << endl;
			cout << "UNEQUAL2a: " << m1.print(po) << ":" << m3.print(po) << endl;
		}*/
		if(m4.isNumber()) testComplex(&m4.number(), m4.number().internalImaginary());
		if(m1.isNumber()) testComplex(&m1.number(), m1.number().internalImaginary());
		if(COMPARISON_IS_NOT_EQUAL(m1.compare(m4)) && !m1.containsInfinity() && !m4.containsInfinity() && !contains_division_by_zero(m1) && !contains_division_by_zero(m4)) {
			rt5++;
			cout << str << " => " << mp << ":" << nr << endl;
			cout << "UNEQUAL2b: " << m1.print(po) << ":" << m4.print(po) << endl;
		}
		if(b_iv != CALCULATOR->usesIntervalArithmetic()) {
			cout << "INTERVAL ARITHMETIC CHANGED4: " << str << " => " << mp << endl;
			CALCULATOR->useIntervalArithmetic(b_iv);
		}
		if(test_interval) {
			m1 = mp;
			if(nr.hasImaginaryPart()) nr = rand() % 100 - 50;
			nr.setPrecision(5);
			Number nr_iv = nr;
			nr_iv.precisionToInterval();
			cerr << "I1: " << nr_iv << endl;
			v->set(nr_iv);
			m1.replace(CALCULATOR->v_x, v);
			m2 = m1;
			eo.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
			CALCULATOR->calculate(&m1, 5000, eo);
			if(m1.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED9"; CALCULATOR->useIntervalArithmetic(b_iv); return;}
			eo.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
			v->set(nr);
			cerr << "I2" << endl;
			CALCULATOR->calculate(&m2, 5000, eo);
			eo.interval_calculation = ic;
			if(m2.isAborted()) {cout << str << " => " << mp << endl; cout << "ABORTED10"; return;}
			if(m1.isNumber() && m2.isNumber() && m1.number().precision(true) > 2) {
				rt8++;
				PrintOptions po;
				po.min_exp = 1;
				po.max_decimals = 2;

				po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				if(m1 != m2) {
					string si1 = m1.number().imaginaryPart().print(po);
					string si2 = m2.number().imaginaryPart().print(po);
					string sr1 = m1.number().realPart().print(po);
					string sr2 = m2.number().realPart().print(po);
					if(si1 != si2 && si1 == "0") {
						nr = m2.number().imaginaryPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							si2 = "0";
						}
					}
					if(sr1 != sr2 && sr1 == "0") {
						nr = m2.number().realPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							sr2 = "0";
						}
					}
					if(si1 != si2 && si2 == "0") {
						nr = m1.number().imaginaryPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							si1 = "0";
						}
					}
					if(sr1 != sr2 && sr2 == "0") {
						nr = m1.number().realPart();
						nr.abs();
						nr.log(10);
						if(nr < -20) {
							sr1 = "0";
						}
					}
					if((sr1 != sr2 && sr1.length() < 10) || (si1 != si2 && si1.length() < 10)) {
						po.max_decimals = 1;
						if(sr1 != sr2) sr2 = m2.number().realPart().print(po);
						if(si1 != si2) si2 = m2.number().imaginaryPart().print(po);
						if((sr1 != sr2 && sr1.length() < 10) || (si1 != si2 && si1.length() < 10)) {
							rt9++;
							cout << str << " => " << mp << ":" << nr_iv << endl;
							cout << "UNEQUAL4: " << m1.print(po) << ":" << m2.print(po) << endl;
						}
					}
				}
			}
		}
	}
	cerr << "D" << endl;
	if(display_errors(true)) cout << str << ":" << str2 << endl;
}

void speed_test() {

	/*UserFunction f1("", "", "x^2/3");
	UserFunction f2("", "", "1/x");
	UserFunction f3("", "", "x/4-3");
	UserFunction f4("", "", "sin(x rad)");

	MathStructure v;
	v.clearVector();
	MathStructure m1 = f1.calculate*/

	//Number nr(1);
	//Number nr_change(1, 8000);
	/*for(size_t i = 0; i < 800000; i++) {
		Number n(nr);
		n ^= 2;
		n /= 3;
		nr += nr_change;
	}
	nr.set(1);
	for(size_t i = 0; i < 800000; i++) {
		Number n(nr);
		n.recip();
		nr += nr_change;
	}
	nr.set(1);
	for(size_t i = 0; i < 800000; i++) {
		Number n(nr);
		n /= 4;
		n -= 3;
		nr += nr_change;
	}*/
	/*nr.set(1);
	for(size_t i = 0; i < 800000; i++) {
		Number n(nr);
		n.sin();
		n *= 2;
		n -= nr;
		n.cos();
		nr += nr_change;
	}*/

	/*mpfr_set_default_prec(32);
	mpfr_t nr;
	mpfr_init(nr);
	mpfr_set_ui(nr, 1, MPFR_RNDN);
	mpfr_t nr_change;
	mpfr_init(nr_change);
	mpfr_set_ui(nr_change, 1, MPFR_RNDN);
	mpfr_div_ui(nr_change, nr_change, 8000, MPFR_RNDN);
	mpfr_t n;
	mpfr_init(n);*/
	/*for(size_t i = 0; i < 800000; i++) {
		mpfr_pow_ui(n, nr, 2, MPFR_RNDN);
		mpfr_div_ui(n, n, 3, MPFR_RNDN);
		mpfr_add(nr, nr, nr_change, MPFR_RNDN);
	}
	mpfr_set_ui(nr, 1, MPFR_RNDN);
	for(size_t i = 0; i < 800000; i++) {
		mpfr_ui_div(n, 1, nr, MPFR_RNDN);
		mpfr_add(nr, nr, nr_change, MPFR_RNDN);
	}
	mpfr_set_ui(nr, 1, MPFR_RNDN);
	for(size_t i = 0; i < 800000; i++) {
		mpfr_div_ui(n, nr, 4, MPFR_RNDN);
		mpfr_sub_ui(n, n, 3, MPFR_RNDN);
		mpfr_add(nr, nr, nr_change, MPFR_RNDN);
	}*/
	/*mpfr_set_ui(nr, 1, MPFR_RNDN);
	for(size_t i = 0; i < 800000; i++) {
		mpfr_sin(n, nr, MPFR_RNDN);
		mpfr_mul_ui(n, nr, 2, MPFR_RNDN);
		mpfr_sub(n, n, nr, MPFR_RNDN);
		mpfr_cos(n, n, MPFR_RNDN);
		mpfr_add(nr, nr, nr_change, MPFR_RNDN);
	}*/

	/*double nr = 1.0;
	double change = 1.0 / 8000;
	double n;
	for(size_t i = 0; i < 800000; i++) {
		n = pow(nr, 2);
		n /= 3;
		nr += change;
	}
	nr = 1.0;
	for(size_t i = 0; i < 800000; i++) {
		n = 1.0 / nr;
		nr += change;
	}
	nr = 1.0;
	for(size_t i = 0; i < 800000; i++) {
		n = nr / 4.0;
		n -= 3.0;
		nr += change;
	}
	nr = 1.0;
	for(size_t i = 0; i < 800000; i++) {
		n = sin(nr);
		nr += change;
	}*/

	EvaluationOptions eo;
	eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	//eo.sync_units = false;

	/*Number n1;
	for(size_t i = 0; i < 4000000; i++) {
		n1 += nr_one;
	}*/

	/*MathStructure m1;
	for(size_t i = 0; i < 1000000; i++) {
		m1.calculateAdd(m_one, eo);
	}*/

	/*for(size_t i = 0; i < 2000000; i++) {
		MathStructure *m1 = new MathStructure();
	}*/

	/*for(size_t i = 0; i < 2000000; i++) {
		Number *m1 = new Number();
	}*/

	/*MathStructure m = CALCULATOR->parse("x^2/2+sin(x+3)^2", eo.parse_options);
	m.eval(eo);
	MathStructure mx(CALCULATOR->v_x);
	Number nr;
	nr.setFloat(4.0);
	MathStructure mnr(nr);
	for(size_t i = 0; i < 1000000; i++) {
		MathStructure r(m);
		r.replace(mx, mnr);
		r.eval(eo);
	}*/
	/*MathStructure m = CALCULATOR->parse("sin('y')+'y'+'y'+'x'^2/3+'x'+'y'", eo.parse_options);
	MathStructure mx("x");
	MathStructure my("y");
	for(size_t i = 0; i < 500000; i++) {
		MathStructure m2(m);
		//m2.replace(mx, m_one);
		//m2.replace(my, m_one);
		m2.replace(mx, m_one, my, m_one);
	}*/
	/*MathStructure m = CALCULATOR->parse("x^2/3", eo.parse_options);
	eo.expand = false;
	m.eval(eo);
	MathStructure v1 = m.generateVector(CALCULATOR->v_x, 1, 100, 800000, NULL, eo);
	m = CALCULATOR->parse("1/x");
	m.eval(eo);
	MathStructure v2 = m.generateVector(CALCULATOR->v_x, 1, 100, 800000, NULL, eo);
	m = CALCULATOR->parse("x/4-3");
	m.eval(eo);
	MathStructure v3 = m.generateVector(CALCULATOR->v_x, 1, 100, 800000, NULL, eo);
	m = CALCULATOR->parse("sin(x)", eo.parse_options);
	m.eval(eo);
	MathStructure v4 = m.generateVector(CALCULATOR->v_x, 1, 100, 800000, NULL, eo);*/
	//cout << v4.size() << ":" << v4[0] << ":" << v4[v4.size() - 1] << endl;
}

Number nr2;
string sbin;

bool test_float(const Number &nr) {
	PrintOptions po;
	po.base = BASE_FP64;
	po.base_display = BASE_DISPLAY_NONE;
	sbin = nr.print(po);
	if(sbin != to_float(nr, 64)) {
		cout << nr << ":" << sbin << ":" << to_float(nr, 64) << endl;
		return false;
	}
	from_float(nr2, sbin, 64);
	if(nr2.print(po) != sbin) {
		if(nr2.isZero() && sbin[0] == '1' && sbin.find("1", 1) == string::npos) return true;
		cout << "B:" << nr << ":" << nr2 << ":" << sbin << endl;
		return false;
	}
	return true;
}

bool contains_abs_or_currency(const MathStructure &m) {
	if(m.isUnit() && (m.unit()->isCurrency() || m.unit()->baseUnit()->referenceName() == "K")) return true;
	if(m.isFunction() && (m.function()->id() == FUNCTION_ID_ABS || m.function()->id() == FUNCTION_ID_BIN || m.function()->id() == FUNCTION_ID_DEC || m.function()->id() == FUNCTION_ID_HEX || m.function()->id() == FUNCTION_ID_BASE || m.function()->id() == FUNCTION_ID_OCT)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_abs_or_currency(m[i])) return true;
	}
	return false;
}

#include "libqalculate/MathStructure-support.h"
int main(int argc, char *argv[]) {

	new Calculator(false);
	//CALCULATOR->loadExchangeRates();
	CALCULATOR->loadGlobalDefinitions();
	CALCULATOR->loadLocalDefinitions();
	CALCULATOR->setPrecision(10);

	CALCULATOR->useIntervalArithmetic();
	PrintOptions po = CALCULATOR->messagePrintOptions();
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.show_ending_zeroes = true;
	po.number_fraction_format = FRACTION_FRACTIONAL;
	po.restrict_fraction_length = true;
	/*po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.show_ending_zeroes = false;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.restrict_fraction_length = true;
	po.min_exp = 1;*/
	//po.max_decimals = 1;
	//po.use_max_decimals = true;
	CALCULATOR->setMessagePrintOptions(po);

	EvaluationOptions evalops;
	//evalops.parse_options.unknowns_enabled = false;
	/*evalops.sync_units = true;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.read_precision = DONT_READ_PRECISION;*/
	/*evalops.parse_options.base = BASE_DECIMAL;
	evalops.allow_complex = true;
	evalops.allow_infinite = true;*/
	//evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
	evalops.assume_denominators_nonzero = true;
	//evalops.warn_about_denominators_assumed_nonzero = true;
	//evalops.parse_options.limit_implicit_multiplication = false;
	//evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	/*evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	evalops.parse_options.comma_as_separator = false;*/
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
	evalops.structuring = STRUCTURING_SIMPLIFY;
	//evalops.approximation = APPROXIMATION_EXACT;

	/*po.use_unicode_signs = true;
	po.number_fraction_format = FRACTION_DECIMAL;
	bool b_approx = false;
	po.is_approximate = &b_approx;
	CALCULATOR->setVariableUnitsEnabled(false);
	for(size_t i = 0; i < 10000; i++) {
		string str = rnd_expression(false, true, 0, 2, false, false, false, false, true);
		cerr << str << endl;
		string str2 = CALCULATOR->calculateAndPrint(str, 1000, evalops, po, AUTOMATIC_FRACTION_AUTO, AUTOMATIC_APPROXIMATION_AUTO, NULL, -1);
		if(str2.find("=") != string::npos || str2.find(SIGN_ALMOST_EQUAL) != string::npos || (!b_approx && str2.find_first_not_of(NUMBER_ELEMENTS) == string::npos)) {
			cout << str << endl;
			cout << str2 << endl << endl;
		}
	}
	for(size_t i = 0; i < 10000; i++) {
		string str = rnd_expression(4, true, 0, 2, false, false, false, false, true);
		cerr << str << endl;
		string str2 = CALCULATOR->calculateAndPrint(str, 1000, evalops, po, AUTOMATIC_FRACTION_AUTO, AUTOMATIC_APPROXIMATION_AUTO, NULL, -1);
		if(str2.find("=") != string::npos || str2.find(SIGN_ALMOST_EQUAL) != string::npos || (!b_approx && str2.find_first_not_of(NUMBER_ELEMENTS) == string::npos)) {
			cout << str << endl;
			cout << str2 << endl << endl;
		}
	}
	return 0;*/

	/*MathStructure m;
	for(size_t i = 0; i < 100000L; i++) {
		CALCULATOR->parse(&m, "mm", evalops.parse_options);
	}
	return 0;*/

	/*MathStructure mstruct = CALCULATOR->calculate("atanh(2x^2+5)*x^2", evalops);
	cout << mstruct.integrate(CALCULATOR->v_x, evalops) << endl;
	mstruct.eval(evalops);
	cout << mstruct << endl;*/
	/*mstruct = CALCULATOR->calculate("atanh(3x-2)*x", evalops);
	mstruct.integrate(CALCULATOR->v_x, evalops);
	mstruct.eval(evalops);
	cout << mstruct << endl;*/
	//speed_test();
	test_integration();
	cout << successes << ":" << imaginary << endl;
	return 0;
	//test_intervals(true);

	/*Number nr;
	evalops.approximation = APPROXIMATION_TRY_EXACT;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.show_ending_zeroes = false;
	while(nr >= -1000) {
	nr.set("-22.49");
	CALCULATOR->setCustomOutputBase(Number("-10"));
	po.base = BASE_CUSTOM;
		string str = nr.print(po);
		po.base = BASE_DECIMAL;
		string expr = "base(";
		expr += str;
		expr += ",-10)";
		string str2 = CALCULATOR->calculateAndPrint(expr, 0, evalops, po);
		cout << nr << ":" << str << ":" << str2 << endl;
		if(nr.print(po) != str2) sleep(1);
		display_errors();
		//nr--;
	}*/
	//return 0;

	CALCULATOR->setVariableUnitsEnabled(false);

	MathStructure mp;
	/*CALCULATOR->parse(&mp, "x + asin(x) + -1 * 2", evalops.parse_options);
	test_integration5(mp, 9, 52);
	return 0;*/
	string str, str2;
	Number a, b;
	/*po.use_min_decimals = true;
	po.use_max_decimals = true;
	po.show_ending_zeroes = true;
	for(size_t i = 0; i < 100000000L; i++) {
		do {
			a.set(rnd_number(false, false, false, false, false));
		} while(a.isZero());
		//a.setApproximate();
		b = a;
		b.exp10();
		b.round();
		//po.min_decimals = i % 16 + b.intValue() + 2;
		//po.max_decimals = i % 16 + 3;
		//if(po.min_decimals > 1 && po.max_decimals > po.min_decimals) po.max_decimals = po.min_decimals;
		po.min_exp = -(i % 3);
		if(po.min_exp == 0) po.min_exp = 1;
		b = a;
		b.setToFloatingPoint();
		po.use_max_decimals = false;
		po.min_decimals = 100;
		b.setRelativeUncertainty(Number(i % 10, 1, -(i % 20) - 3));
		str2 = b.print(po);
		po.use_max_decimals = true;
		size_t p1 = str2.find(".");
		size_t p2 = str2.find("E");
		if(p2 == string::npos) p2 = str2.length();
		else if(po.min_exp == -1) po.min_exp = 1;
		if(p1 == string::npos) {
			po.max_decimals = 0;
		} else {
			po.max_decimals = p2 - p1 - 1;
		}
		po.min_decimals = po.max_decimals;
		str = a.print(po);
		if(str != str2) {
			cout << str << ":" << str2 << ":" << po.min_decimals << ":" << po.max_decimals << endl;
			po.interval_display = INTERVAL_DISPLAY_INTERVAL;
			//po.min_decimals = 20;
			//po.max_decimals = -1;
			cout << a.print(po) << ":" << b.print(po) << endl;
			po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		}
	}
	return 0;*/
	/*for(size_t i = 0; i < 50000;) {
		str = rnd_expression(10, true, 6, 4, false, false, false, false, false, 5, false);
		CALCULATOR->parse(&mp, str, evalops.parse_options);
		cerr << str << endl;
		if(mp.contains(CALCULATOR->v_x)) {
			a.set(rnd_number(false, false, false, false, false));
			b.set(rnd_number(false, false, false, false, false));
			cerr << "A" << endl;
			if(a < b) test_integration5(mp, a, b);
			else test_integration5(mp, b, a);
			i++;
			if(i % 1000 == 0) cout << f1 << ":" << s1 << ":" << s2 << ":" << successes << ":" << imaginary << endl;
		}
	}
	cout << successes << ":" << imaginary << endl;
	return 0;*/

	/*Number nr;
	for(long int i = 0; i < 1000000L; i++) {
		nr.set(rnd_number(false, false, false, false, false));
		if(i % 2 == 1) nr.exp2(rand() % 2200 - 1100);
		if(!test_float(nr)) return 0;
	}
	Number nr2;
	for(long int i = 0; i < 1000000L; i++) {
		nr.rand();
		nr2.rand();
		nr /= nr2;
		if(i % 2 == 1) nr.exp2(rand() % 2200 - 1100);
		nr.intervalToMidValue();
		if(!test_float(nr)) return 0;
	}

	return 0;*/

	v = new KnownVariable("", "v", m_zero);

	//CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NUMBER);
	//CALCULATOR->useIntervalArithmetic();

	//for(size_t i = 0; i <= 150000; i++) {
		/*string str = rnd_expression(17, false, 20, 4, false, false, false, false, true);
		cout << str << endl;
		MathStructure mstruct;
		CALCULATOR->calculate(&mstruct, str, 10000, evalops);
		mstruct.format(po);
		cout << mstruct.print() << endl;
		if(mstruct.isAborted()) break;*/
		//if(mstruct.isPower() || (mstruct.isMultiplication() && !mstruct.containsType(STRUCT_DIVISION))) cout << str << "\n" << mstruct << endl;
	/*	rnd_test(evalops, 4, true, false, true, false, true, false);
		if(i % 1000 == 0) cout << endl << rt1 << ":" << rt2 << ":" << rt3 << ":" << rt4 << ":" << rt5 << ":" << rt6 << ":" << rt7 << ":" << rt8 << ":" << rt9 << endl << endl;
	}
	cout << endl << endl << "-----------------------------------------" << endl << endl << endl;

	return 0;*/
	evalops.parse_options.units_enabled = true;
	evalops.parse_options.unknowns_enabled = false;
	size_t ni = 0;
	for(size_t i2 = 0; i2 <= 10000000L; i2++) {
		str = "";
		size_t n = rand() % 200;
		for(size_t i = 0; i <= n; i++) {
			str += (char) (rand() % (126 - 32) + 32);
			//if(str[i] == '{' || str[i] == '}') str[i] = '+';
		}
		while(true) {
			n = rand() % 12;
			if(n > 6) break;
			size_t i = rand() % str.length();
			if(n == 0) str.insert(i, "(");
			if(n == 1) str.insert(i, ")");
			if(n == 2) str.insert(i, "+");
			if(n == 3) str.insert(i, "*");
			if(n == 4) str.insert(i, "/");
			if(n == 5) str.insert(i, "^");
			if(n == 6) str.insert(i, "-");
			//if(n == 7) str.insert(i, "~");
		}
		gsub("!", " ", str);
		/*gsub("{", " ", str);
		gsub("}", " ", str);*/
		gsub("~", " ", str);
		gsub(":=", "=", str);
		gsub("=:", "=", str);
		gsub(">", " ", str);
		gsub("<", " ", str);
		gsub(":", " ", str);
		gsub("=", " ", str);
		remove_blank_ends(str);
		while(str[0] == '/') {str.erase(0, 1); remove_blank_ends(str);}
		cout << "\n\n\n\n\n" << str << endl;
		int max_length = 100 - unicode_length(str);
		if(max_length < 50) max_length = 50;
		string parsed;
		MathStructure mstruct;
		//CALCULATOR->parse(&mstruct, str, evalops.parse_options);
		//mstruct.eval(evalops);
		//if(CALCULATOR->idCount() > 0) break;
		//cout << mstruct.print() << endl;
		str = CALCULATOR->calculateAndPrint(str, 100, evalops, default_print_options, AUTOMATIC_FRACTION_AUTO, AUTOMATIC_APPROXIMATION_AUTO, &parsed, max_length);
		/*CALCULATOR->startControl(1000);
		CALCULATOR->calculate(str, evalops);
		CALCULATOR->stopControl();*/
		if(str.find("tiden rann ut") != string::npos) {
			if(str != "tiden rann ut") {
				cout << str << endl;
				break;
			}
			ni++;
		}
		//cout << parsed << endl;
		/*n = rand() % (str.length() + 1);
		if(n < str.length()) str.insert(n, "=");
		for(size_t i3 = 0; i3 < 7; i3++) {
			n = rand() % 3;
			if(n == 0) {
				size_t n2 = rand() % 4;
				n = rand() % str.length();
				while(n > 0 && n < str.length() - 1 && (signed char) str[n] < 0 && (signed char) str[n - 1] < 0) n--;
				if(n2 == 0) str.insert(n, "\"");
				if(n2 == 1) str.insert(n, "\'");
				if(n2 == 2) str.insert(n, "«");
				if(n2 == 3) str.insert(n, "›");
			}
		}*/
		/*CALCULATOR->parse(&mstruct, str, evalops.parse_options);
		bool b_out = true;
		if(expression_contains_save_function(str, evalops.parse_options, false)) b_out = false;
		if(mstruct.isSymbolic() || contains_abs_or_currency(mstruct)) b_out = false;
		while(CALCULATOR->message()) {
			if(CALCULATOR->message()->message().find("is not a valid") != string::npos || CALCULATOR->message()->message().find("Trailing") != string::npos || CALCULATOR->message()->message().find("Misplaced") != string::npos || CALCULATOR->message()->message().find("ignored") != string::npos) {b_out = false; break;}
			if(!CALCULATOR->nextMessage()) break;
		}
		if(b_out && !str.empty()) cout << str << endl;*/
		CALCULATOR->clearMessages();
		//cerr << str << endl;
		/*cout << mstruct.print() << endl;
		mstruct.eval(evalops);
		cout << "B" << endl;
		cout << "C" << endl;
		cout << mstruct.print() << endl;
		cout << "D" << endl;
		CALCULATOR->convertToOptimalUnit(mstruct, evalops, true);
		cout << "E" << endl;*/
		/*expression_contains_save_function(str, evalops.parse_options);
		if(transform_expression_for_equals_save(str, evalops.parse_options)) {
			ni++;
			cout << "SAVE:" << str << endl;
		}*/
		/*CALCULATOR->calculate(&mstruct, str, 10000, evalops);
		if(mstruct.isAborted()) {cerr << "aborted: " << i2 << endl; break;}
		display_errors(true);
		mstruct.format(po);
		cerr << mstruct.print() << endl;
		if(mstruct.isAborted()) break;*/
		/*for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i]->isLocal()) {
				cout << "G" << endl;
				CALCULATOR->variables[i]->destroy();
			}
		}*/
		cout << i2 << endl;
	}
	cerr << ni<< endl;
	return 0;

}

