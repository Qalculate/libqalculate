# libqalculate
Qalculate! library and CLI

![Image of qalc](http://qalculate.github.io/images/qalc.png)

Qalculate! is a multi-purpose desktop calculator for GNU/Linux (and Mac OS). It is small and simple to use but with much power and versatility underneath. Features include customizable functions, units, arbitrary precision, plotting, and a user-friendly interface (GTK+).

Features if libqalculate:

* Calculation and parsing:
   * Basic operations and operators: + - * / ^ E () && || ! < > >= <= != ~ & | << >>
   * Fault-tolerant parsing of strings: log 5 / 2 .5 (3) + (2( 3 +5 = ln(5) / (2.5 * 3) + 2 * (3 + 5)
   * Supports complex and infinite numbers
   * Supports all number bases from 2 to 36, time format and roman numerals
   * Ability to disable functions, variables, units or unknown variables for less confusion: ex. when you do not want (a+b)^2 to mean (are+barn)^2 but ("a"+"b")^2
   * Controllable implicit multiplication
   * Matrices and vectors, and related operations (determinants etc.)
   * Verbose error messages
   * Arbitrary precision
   * RPN mode
* Result display:
   * Supports all number bases from 2 to 36, plus sexagesimal numbers, time format and roman numerals
   * Many customization options: precision, max/min decimals, multiplication sign, etc.
   * Exact or approximate
   * Fractions: 4 / 6 * 2 = 1.333... = 4/3 = 1 + 1/3
* Symbolic calculation:
   * Ex. (x + y)^2 = x^2 + 2xy + y^2; 4 "apples" + 3 "oranges"
   * Factorization and simplification
   * Differentiation and integration
   * Can solve most equations and inequalities
   * Customizable assumptions give different results (ex. ln(2x) = ln(2) + ln(x) if x is assumed positive)
* Functions:
   * All the usual functions: sine, log, etc... : ln 5 = 1.609; sqrt(tan(20) - 5) = sqrt(-2.76283905578)
   * Lots of statistical, financial, geometrical, and more functions (approx. 200)
   * If..then..else function, optional arguments and more features for flexible function creation
   * Can easily be created, edit and saved to a standard XML file
* Units:
   * Supports all SI units and prefixes (including binary), as well as imperial and other unit systems
   * Automatic conversion: ft + yd + m = 2.2192 m
   * Implicit conversion: 5m/s to mi/h = 11.18 miles/hour
   * Smart conversion: can automatically convert 5 kg*m/s^2 to 5 newton
   * Currency conversion with retrieval of daily exchange rates
   * Different name forms: abbreviation, singular, plural (m, meter, meters)
   * Can easily be created, edit and saved to a standard XML file
* Variables and constants:
   * Basic constants: pi, e
   * Lots of physical constants and elements
   * CSV file import and export
   * Can easily be created, edit and saved to a standard XML file
   * Flexible, can contain simple numbers, units or whole expressions
   * Data sets with objects and associated properties in database-like structure
* Plotting:
   * Uses Gnuplot
   * Can plot functions or data (matrices and vectors)
   * Ability to save plot to PNG image, postscript, etc.
   * Several customization options
* and more...


