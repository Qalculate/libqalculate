#include <libqalculate/qalculate.h>

int main(int argc, char *argv[]) {

	new Calculator();
	CALCULATOR->loadGlobalDefinitions();
	
	CALCULATOR->setPrecision(10);

#define TEST_INTERVALS

	CALCULATOR->useIntervalArithmetic(true);
	PrintOptions po;
	//po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	
	vector<Number> nrs;
#ifdef TEST_INTERVALS
	nrs.push_back(nr_plus_inf);
	nrs.push_back(nr_minus_inf);
#endif
	nrs.push_back(nr_zero);
	nrs.push_back(nr_half);
	nrs.push_back(nr_minus_half);
	nrs.push_back(nr_one);
	nrs.push_back(nr_minus_one);
	nrs.push_back(nr_two);
	Number nr;
#ifdef TEST_INTERVALS
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
#else
#define INCLUDES_INFINITY(x) (false)
#define IS_INTERVAL(x) (false)
#endif
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
	return 0;
	
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
	cout << "5: " << nrm.print() << endl;
	
	return 0;*/

}
