#include <libqalculate/qalculate.h>

void display_errors() {
	if(!CALCULATOR->message()) return;
	while(true) {
		MessageType mtype = CALCULATOR->message()->type();
		if(mtype == MESSAGE_ERROR) cout << "error: ";
		else if(mtype == MESSAGE_WARNING) cout << "warning: ";
		cout << CALCULATOR->message()->message() << endl;
		if(!CALCULATOR->nextMessage()) break;
	}
}
void clear_errors() {
	if(!CALCULATOR->message()) return;
	while(true) {
		if(!CALCULATOR->nextMessage()) break;
	}
}

void test_integration4(const MathStructure &mstruct) {
	MathStructure x_var(CALCULATOR->v_x);
	cout << "Integration test: " << mstruct.print() << endl;
	MathStructure mstruct2(mstruct);
	EvaluationOptions eo;
	eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	eo.assume_denominators_nonzero = true;
	mstruct2.integrate(x_var, eo, true, 1);
	mstruct2.eval(eo);
	if(mstruct2.containsFunction(CALCULATOR->f_integrate)) {clear_errors(); return;}
	mstruct2.differentiate(x_var, eo);
	MathStructure mstruct3(mstruct2);
	mstruct3.eval(eo);
	mstruct3.replace(x_var, 3);
	mstruct3.eval(eo);
	display_errors();
	string str1 = mstruct3.print();
	cout << str1 << endl;
	mstruct3 = mstruct;
	mstruct3.replace(x_var, 3);
	mstruct3.eval(eo);
	display_errors();
	string str2 = mstruct3.print();
	cout << str2 << endl;
	if(str1 != str2) cout << "!!!" << endl;
	mstruct3 = mstruct2;
	mstruct3.replace(x_var, -5);
	mstruct3.eval(eo);
	display_errors();
	str1 = mstruct3.print();
	cout << str1 << endl;
	mstruct3 = mstruct;
	mstruct3.replace(x_var, -5);
	mstruct3.eval(eo);
	display_errors();
	str2 = mstruct3.print();
	cout << str2 << endl;
	if(str1 != str2) cout << "!!!" << endl;
	cout << "________________________________________________" << endl;
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

string rnd_number() {
	string str;
	bool par = false;
	bool dot = false;
	bool started = false;
	if(rand() % 3 == 0) {str += '-'; par = true;}
	while(true) {
		int r = rand();
		if(!started) r = r % 10 + 2;
		else if(str.back() == '.') r = r % 10 + 1;
		else r = r % ((dot ? 20 : 21) + str.length() * 10) + 1;
		if(r > (dot ? 10 : 11)) break;
		if(r == 11) {if(!started) str += '0'; str += '.'; dot = true;}
		else str += '0' + (r - 1);
		started = true;
	}
	if(rand() % 10 == 0) {str += 'i'; par = true;}
	if(par) {str += ')'; str.insert(0, "(");}
	return str;
}

string rnd_item(int &par, bool allow_function = true, int allow_unknown = 1) {
	int r = rand() % (2 + (allow_unknown > 0)) + 1;
	string str;
	if(r != 1) {
		str = rnd_number();
	} else {
		int au2 = 3 - allow_unknown % 3;
		r = (rand() % ((allow_function ? 21 : 5) - au2)) + 4 - allow_unknown;
		if(r < 4 - allow_unknown % 3) {
			if(r < 0) r = -r;
			if(allow_unknown % 3 == 1) r = 4;
			else if(allow_unknown % 3 == 2) r = 2 + r % 2;
			else r = 1 + r % 3;
		}
		switch(r) {
			case 1: {str = "z"; break;}
			case 2: {str = "y"; break;}
			case 3: {str = "x"; break;}
			case 4: {str = "pi"; break;}
			case 5: {str = "e"; break;}
			case 6: {par++; str = "sin("; return str + rnd_item(par, true, allow_unknown);}
			case 7: {par++; str = "cos("; return str + rnd_item(par, true, allow_unknown);}
			case 8: {par++; str = "tan("; return str + rnd_item(par, true, allow_unknown);}
			case 9: {par++; str = "sinh("; return str + rnd_item(par, true, allow_unknown);}
			case 10: {par++; str = "cosh("; return str + rnd_item(par, true, allow_unknown);}
			case 11: {par++; str = "tanh("; return str + rnd_item(par, true, allow_unknown);}
			case 12: {par++; str = "asin("; return str + rnd_item(par, true, allow_unknown);}
			case 13: {par++; str = "acos("; return str + rnd_item(par, true, allow_unknown);}
			case 14: {par++; str = "atan("; return str + rnd_item(par, true, allow_unknown);}
			case 15: {par++; str = "asinh("; return str + rnd_item(par, true, allow_unknown);}
			case 16: {par++; str = "acosh("; return str + rnd_item(par, true, allow_unknown);}
			case 17: {par++; str = "atanh("; return str + rnd_item(par, true, allow_unknown);}
			case 18: {par++; str = "ln("; return str + rnd_item(par, true, allow_unknown);}
			case 19: {par++; str = "abs("; return str + rnd_item(par, true, allow_unknown);}
			case 20: {par++; str = "sqrt("; return str + rnd_item(par, true, allow_unknown);}
			case 21: {par++; str = "cbrt("; return str + rnd_item(par, true, allow_unknown);}
		}
	}
	r = rand() % (5 * (par + 1)) + 1;
	if(r == 1) {
		str.insert(0, "(");
		par++;
	}
	return str;
}
string rnd_operator(int &par) {
	int r = rand() % (par ? 10 : 5) + 1;
	string str;
	if(par > 0 && r > 5) {
		par--;
		str = ")";
		r -= 5;
	}
	if(r == 5 && rand() % 3 != 0) r = rand() % 4 + 1;
	switch(r) {
		case 1: return str + "+";
		case 2: return str + "-";
		case 3: return str + "*";
		case 4: return str + "/";
		case 5: return str + "^";
	}
	return "";
}

int rt1 = 0, rt2 = 0, rt3 = 0, rt4 = 0, rt5 = 0, rt6 = 0, rt7 = 0;
void rnd_test(EvaluationOptions eo, int allow_unknowns, bool allow_functions) {
	int par = 0;
	string str;
	while(str.empty() || rand() % (str.length() > 40 ? 2 : 10 - str.length() / 5) != 0) {
		str += rnd_item(par, allow_functions, allow_unknowns);
		str += rnd_operator(par);
	}
	if(str.back() != ')') str += rnd_item(par, false, allow_unknowns);
	MathStructure mp, m1, m2;
	CALCULATOR->parse(&mp, str, eo.parse_options);
	eo.approximation = APPROXIMATION_APPROXIMATE;
	m1 = mp;
	CALCULATOR->calculate(&m1, 1000, eo);
	if(m1.isAborted()) {cout << str << endl; cout << "ABORTED1" << endl; return;}
	eo.approximation = APPROXIMATION_EXACT;
	m2 = mp;
	CALCULATOR->calculate(&m2, 1000, eo);
	if(m2.isAborted()) {cout << str << endl; cout << "ABORTED2" << endl; return;}
	eo.approximation = APPROXIMATION_APPROXIMATE;
	CALCULATOR->calculate(&m2, 1000, eo);
	if(m2.isAborted()) {cout << str << endl; cout << "ABORTED3" << endl; return;}
	if(m1.isNumber() || m2.isNumber()) {
		rt1++;
		if(m1 != m2 && m1.print() != m2.print()) {
			rt2++;
			cout << str << endl;
			cout << "UNEQUAL1: " << m1.print() << ":" << m2.print() << endl;
		}
	}
	if(mp.contains(CALCULATOR->v_x)) {
		rt3++;
		Number nr(rnd_number());
		if(nr.hasImaginaryPart() && rand() % 2 == 0) nr += Number(rnd_number());
		m1 = mp;
		m1.replace(CALCULATOR->v_x, nr);
		CALCULATOR->calculate(&m1, 1000, eo);
		if(m1.isAborted()) {cout << str << endl; cout << "ABORTED4: " << nr << endl; return;}
		m2 = mp;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		CALCULATOR->calculate(&m2, 1000, eo);
		if(m2.isAborted()) {cout << str << endl; cout << "ABORTED5: " << nr << endl; return;}
		m2.replace(CALCULATOR->v_x, nr);
		eo.approximation = APPROXIMATION_APPROXIMATE;
		CALCULATOR->calculate(&m2, 1000, eo);
		if(m2.isAborted()) {cout << str << endl; cout << "ABORTED6: " << nr << endl; return;}
		if(m1.isNumber() || m2.isNumber()) {
			rt4++;
			if(m1 != m2 && m1.print() != m2.print()) {
				rt5++;
				cout << str << endl;
				cout << "UNEQUAL2: " << m1.print() << ":" << m2.print() << endl;
			}
		}
		
		m1 = mp;
		m1.transform(COMPARISON_EQUALS, m_zero);
		cout << m1 << endl;
		eo.approximation = APPROXIMATION_EXACT;
		CALCULATOR->calculate(&m1, 1000, eo);
		if(m1.isAborted()) {cout << str << endl; cout << "ABORTED7"; return;}
		if(m1.isComparison() && m1.comparisonType() == COMPARISON_EQUALS && m1[0] == CALCULATOR->v_x) {
			m2 = mp;
			m2.replace(CALCULATOR->v_x, m1[1]);
			eo.approximation = APPROXIMATION_APPROXIMATE;
			CALCULATOR->calculate(&m2, 1000, eo);
			if(m2.isAborted()) {cout << str << endl; cout << "ABORTED8: " << m1[1] << endl; return;}
			if(m2.isNumber()) {
				rt6++;
				if(m2.number().isNonZero()) {
					rt7++;
					nr = m2.number();
					nr.abs();
					nr.log(10);
					if(nr > -30) {
						cout << str << endl;
						cout << "UNEQUAL3: " << m1.print() << ":" << m2.print() << endl;
					}
				}
			}
		} else if(!m1.isComparison()) {
			for(size_t i = 0; i < m1.size(); i++) {
				if(m1[i].isComparison() && m1[i].comparisonType() == COMPARISON_EQUALS && m1[i][0] == CALCULATOR->v_x) {
					m2 = mp;
					m2.replace(CALCULATOR->v_x, m1[i][1]);
					eo.approximation = APPROXIMATION_APPROXIMATE;
					CALCULATOR->calculate(&m2, 1000, eo);
					if(m2.isAborted()) {cout << str << endl; cout << "ABORTED8: " << m1[i][1] << endl; return;}
					if(m2.isNumber()) {
						rt6++;
						if(m2.number().isNonZero()) {
							nr = m2.number();
							nr.abs();
							nr.log(10);
							if(nr > -30) {
								rt7++;
								cout << str << endl;
								cout << "UNEQUAL3: " << m1[i].print() << ":" << m2.print() << endl;
							}
						}
					}
				} else if(!m1[i].isComparison()) {
					for(size_t i2 = 0; i2 < m1[i].size(); i2++) {
						if(m1[i][i2].isComparison() && m1[i][i2].comparisonType() == COMPARISON_EQUALS && m1[i][i2][0] == CALCULATOR->v_x) {
							m2 = mp;
							m2.replace(CALCULATOR->v_x, m1[i][i2][1]);
							eo.approximation = APPROXIMATION_APPROXIMATE;
							CALCULATOR->calculate(&m2, 1000, eo);
							if(m2.isAborted()) {cout << str << endl; cout << "ABORTED8: " << m1[i][i2][1] << endl; return;}
							if(m2.isNumber()) {
								rt6++;
								if(m2.number().isNonZero()) {
									nr = m2.number();
									nr.abs();
									nr.log(10);
									if(nr > -30) {
										rt7++;
										cout << str << endl;
										cout << "UNEQUAL3: " << m1[i][i2].print() << ":" << m2.print() << endl;
									}
								}
							}
						}
					}
				}
			}
		}
	}
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

int main(int argc, char *argv[]) {

	new Calculator();
	CALCULATOR->loadGlobalDefinitions();
	CALCULATOR->loadLocalDefinitions();
	
	EvaluationOptions evalops;
	/*evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_SIMPLIFY;
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
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;*/
	
	/*MathStructure mstruct = CALCULATOR->calculate("atanh(2x^2+5)*x^2", evalops);
	cout << mstruct.integrate(CALCULATOR->v_x, evalops) << endl;
	mstruct.eval(evalops);
	cout << mstruct << endl;*/
	/*mstruct = CALCULATOR->calculate("atanh(3x-2)*x", evalops);
	mstruct.integrate(CALCULATOR->v_x, evalops);
	mstruct.eval(evalops);
	cout << mstruct << endl;*/
	//speed_test();
	//test_integration();
	//test_intervals(true);
	
	for(size_t i = 0; i < 1000; i++) {
		rnd_test(evalops, 4, true);
	}
	cout << rt1 << ":" << rt2 << ":" << rt3 << ":" << rt4 << ":" << rt5 << ":" << rt6 << ":" << rt7 << endl;

	return 0;

}

