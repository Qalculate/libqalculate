# libqalculate
Qalculate! library and CLI

![Image of qalc](http://qalculate.github.io/images/qalc.png)

Qalculate! is a multi-purpose cross-platform desktop calculator. It is simple to use but provides power and versatility normally reserved for complicated math packages, as well as useful tools for everyday needs (such as currency conversion and percent calculation). Features include a large library of customizable functions, unit calculations and conversion, symbolic calculations (including integrals and equations), arbitrary precision, uncertainty propagation, interval arithmetic, plotting, and a user-friendly interface (GTK+ and CLI).

## Installation
Instructions and download links for installers, binaries packages, and the source code of released versions of Qalculate! are available at https://qalculate.github.io/downloads.html.

In a terminal window in the top source code directory run
* `./autogen.sh` *(not required if using a release source tarball, only if using the git version)*
* `./configure`
* `make`
* `make install` *(as root, e.g. `sudo make install`)*
* `ldconfig` *(if necessary, as root)*
If libqalculate is installed in /usr/local (default) you may need to add /usr/local/lib to the library path of the system (add /usr/local/lib to a file under /etc/ld.so.conf.d/ and run ldconfig).

## Requirements
* GMP and MPFR
* libxml2
* libcurl, icu, gettext (recommended)
* iconv, readline (recommended for CLI)
* Gnuplot (optional)
* doxygen (for compilation of git version)

## API Documentation
The API documentation is included in the package and is installed in $docdir/libqalculate/html (usually /usr/share/doc/libqalculate/html). It is generated when running autogen.sh.

It is also available online at http://qalculate.github.io/reference/index.html.

## Using the CLI program 'qalc'
To calculate a single expression from the command line (non-interactive mode) enter
`qalc mathematical expression` *(e.g. qalc 5+2)*

`qalc --help` shows information about command line options in non-interactive mode.

If you run `qalc` without any mathematical expression the program will start in interactive mode, where you can enter multiple expressions with history and completion, manipulate the result and change settings. Type `help` in interactive mode for more information.

## Other Applications
The main user interface for libqalculate is qalculate-gtk (https://github.com/Qalculate/qalculate-gtk).

Other software using libqalculate include
* KDE Plasma Workspace (https://www.kde.org/workspaces/plasmadesktop/)
* Cantor (http://kde.org/applications/education/cantor/)
* Step (http://kde.org/applications/education/step/)
* Qalculate widget for KDE Plasma (https://store.kde.org/p/1155946/)

## Features
* Calculation and parsing:
   * Basic operations and operators: + - * / mod ^ E () && || ! < > >= <= != ~ & | << >> xor
   * Fault-tolerant parsing of strings: log 5 / 2 .5 (3) + (2( 3 +5 = ln(5) / (2.5 * 3) + 2 * (3 + 5)
   * Expressions may contain any combination of numbers, functions, units, variables, vectors and matrices, and dates
   * Supports complex and infinite numbers
   * Propagation of uncertainty
   * Interval arithmetic (for determination of the number of significant digits or direct calculation with intervals of numbers)
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
   * Ability to disable functions, variables, units or unknown variables for less confusion: e.g. when you do not want (a+b)^2 to mean (are+barn)^2 but ("a"+"b")^2
   * Controllable implicit multiplication
   * Matrices and vectors, and related operations (determinants etc.)
   * Verbose error messages
   * Arbitrary precision
   * RPN mode
* Result display:
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
   * Many customization options: precision, max/min decimals, complex form, multiplication sign, etc.
   * Exact or approximate: sqrt(32) returns 4 * sqrt(2) or 5.66
   * Simple and mixed fractions: 4 / 6 * 2 = 1.333... = 4/3 = 1 + 1/3
* Symbolic calculation:
   * E.g. (x + y)^2 = x^2 + 2xy + y^2; 4 "apples" + 3 "oranges"
   * Factorization and simplification
   * Differentiation and integration
   * Can solve most equations and inequalities
   * Customizable assumptions give different results (e.g. ln(2x) = ln(2) + ln(x) if x is assumed positive)
* Functions:
   * Hundreds of flexible functions: trigonometry, exponents and logarithms, combinatorics, geometry, calculus, statistics, finance, time and date, etc.
   * Can easily be created, edited and saved to a standard XML file
* Units:
   * Supports all SI units and prefixes (including binary), as well as imperial and other unit systems
   * Automatic conversion: ft + yd + m = 2.2192 m
   * Explicit conversion: 5 m/s to mi/h = 11.18 miles/hour
   * Smart conversion: automatically converts 5 kg*m/s^2 to 5 N
   * Currency conversion with retrieval of daily exchange rates
   * Different name forms: abbreviation, singular, plural (m, meter, meters)
   * Can easily be created, edited and saved to a standard XML file
* Variables and constants:
   * Basic constants: pi, e, etc.
   * Lots of physical constants (with or without units) and properties of chemical element
   * CSV file import and export
   * Can easily be created, edited and saved to a standard XML file
   * Flexible - may contain simple numbers, units, or whole expressions
   * Data sets with objects and associated properties in database-like structure
* Plotting:
   * Uses Gnuplot
   * Can plot functions or data (matrices and vectors)
   * Ability to save plot to PNG image, postscript, etc.
   * Several customization options
* and more...

_For more details about the syntax, and available functions, units, and variables, please consult the manual (https://qalculate.github.io/manual/)_

## Examples (expressions)

_Note that semicolon can be replaced with comma, if comma is not used as decimal or thousands separator._

### Basic functions and operators

sqrt 4 _= sqrt(4) = 4^(0.5) = 4^(1/2) = 2_

sqrt(25; 16; 9; 4) _= \[5; 4; 3; 2\]_

sqrt(32) _= 4 × √(2) (in exact mode)_

cbrt(−27) _= root(-27; 3) = −3 (real root)_

(−27)^(1/3) _≈ 1.5 + 2.5980762i (principal root)_

ln 25 _= log(25; e) ≈ 3.2188758_

log2(4)/log10(100) _= log(4; 2)/log(100; 10) = 1_

5! _= 1 × 2 × 3 × 4 × 5 = 120_

5\2 _= 5//2 = trunc(5 / 2) = 2 (integer division)_

5 mod 3 _= mod(5; 3) = 2_

52 to factors _= 2^2 × 13_

25/4 × 3/5 to fraction _= 3 + 3/4_

gcd(63; 27) _= 9_

sin(pi/2) − cos(pi) _= sin(90 deg) − cos(180 deg) = 2_

sum(x; 1; 5) _= 1 + 2 + 3 + 4 + 5 = 15_

sum(\i^2+sin(\i); 1; 5; \i) _= 1^2 + sin(1) + 2^2 + sin(2) + ... ≈ 55.176162_

product(x; 1; 5) _= 1 × 2 × 3 × 4 × 5 = 120_

var1:=5 _(stores value 5 in variable var1)_
var1 × 2 _= 10_

5^2 #this is a comment _= 25_

sinh(0.5) where sinh()=cosh() _= cosh(0.5) ≈ 1.1276260_

plot(x^2; −5; 5) _(plots the function y=x^2 from -5 to 5)_

### Units

5 dm3 to L _= 25 dm^3 to L = 5 L_

20 miles / 2h to km/h _= 16.09344 km/h_

1.74 to ft _= 1.74 m to ft ≈ 5 ft + 8.5039370 in_

1.74 m to -ft _≈ 5.7086614 ft_

100 lbf × 60 mph to hp _≈ 16 hp_

50 Ω × 2 A _= 100 V_

50 Ω × 2 A to base _= 100 kg·m²/(s³·A)_

10 N / 5 Pa _= (10 N)/(5 Pa) = 2 m²_

5 m/s to s/m _= 0.2 s/m_

500 € − 20% to $ _≈ $451.04_

500 megabit/s × 2 h to b?byte _≈ 419.09516 gibibytes_

### Physical constants

k\_e / G × a\_0 _= (coulombs\_constant / newtonian\_constant) × bohr\_radius ≈ 7.126e9 kg·H·m^−1_

ℎ / (λ\_C × c) _= planck ∕ (compton\_wavelength × speed\_of\_light) ≈ 9.1093837e-31 kg_

5 ns × rydberg to c _≈ 6.0793194E-8c_

atom(Hg; weight) + atom(C; weight) × 4 to g _≈ 4.129e-22 g_

(G × planet(earth; mass) × planet(mars; mass))/(54.6e6 km)^2 _≈ 8.58e16 N (gravitational attraction between earth and mars)_

### Uncertainty and interval arithmetic

_"±" can be replaced with "+/-"; result with interval arithmetic activated is shown in parenthesis_

sin(5±0.2)^2/2±0.3 _≈ 0.460±0.088 (0.46±0.12)_

(2±0.02 J)/(523±5 W) _≈ 3.824±0.053 ms (3.825±0.075 ms)_

interval(−2; 5)^2 _≈ intervall(−8.2500000; 12.750000) (intervall(0; 25))_

### Algebra

(5x^2 + 2)/(x − 3) _= 5x + 15 + 47/(x − 3)_

(\a + \b)(\a − \b) _= ("a" + "b")("a" − "b") = 'a'^2 − 'b'^2_

(x + 2)(x − 3)^3 _= x^4 − 7x^3 + 9x^2 + 27x − 54_

factorize x^4 − 7x^3 + 9x^2 + 27x − 54 _= x^4 − 7x^3 + 9x^2 + 27x − 54 to factors = (x + 2)(x − 3)^3_

cos(x)+3y^2 where x=pi and y=2 _= 11_

gcd(25x; 5x^2) _= 5x_

1/(x^2+2x−3) to partial fraction _= 1/(4x − 4) − 1/(4x + 12)_

x+x^2+4 = 16
_= x = 3 or x = −4_

x^2/(5 m) − hypot(x; 4 m) = 2 m where x>0
_x ≈ 7.1340411 m_

cylinder(20cm; x) = 20L _(calculates the height of a 20 L cylinder with radius of 20 cm)_
_= x = (1 / (2π)) m_
_= x ≈ 16 cm_

asin(sqrt(x)) = 0.2
_= x = sin(0.2)^2_
_= x ≈ 0.039469503_

x^2 > 25x
_= x > 25 or x < 0_

solve(x = y+ln(y); y) _= lambertw(e^x)_

solve2(5x=2y^2; sqrt(y)=2; x; y) _= 32/5_

multisolve(\[5x=2y+32; y=2z; z=2x\]; \[x; y; z\]) _= \[−32/3; −128/3; −64/3\]_

dsolve(diff(y; x) − 2y = 4x; 5) _= 6e^(2x) − 2x − 1_

### Calculus

diff(6x^2) _= 12x_

diff(sinh(x^2)/(5x) + 3xy/sqrt(x)) _= (2/5) × cosh(x^2) − sinh(x^2)/(5x^2) + (3y)/(2 × √(x))_

integrate(6x^2) _= 2x^3 + C_

integrate(6x^2; 1; 5) _= 248_

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x)) _= 2x × √(x) × y + Shi(x^2) / 10 + C_

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x); 1; 2) _≈ 3.6568542y + 0.87600760_

limit(ln(1 + 4x)/(3^x − 1); 0) _= 4 / ln(3)_

### Matrices and vectors

((1; 2; 3); (4; 5; 6)) _= \[\[1; 2; 3\]; \[4; 5; 6\]\] (2×3 matrix)_

(1; 2; 3) × 2 − 2 _= \[1 × 2 − 2; 2 × 2 − 2; 3 × 2 − 2\] = \[0; 2; 4\]_

(1; 2; 3) × (4; 5; 6) _= 32 (dot product)_

cross((1; 2; 3); (4; 5; 6)) _= \[−3; 6; −3\] (cross product)_

((1; 2; 3); (4; 5; 6)) × ((7; 8); (9; 10); (11; 12)) _= \[\[58; 64\]; \[139; 154\]\]_

hadamard(\[\[1; 2; 3\]; \[4; 5; 6\]\]; \[\[7; 8; 9\]; \[10; 11; 12\]\]) _= \[\[7; 16; 27\]; \[40; 55; 72\]\] (hadamard product)_

((1; 2); (3; 4))^-1 _= inverse(\[\[1; 2\]; \[3; 4\]\]) = \[\[−2; 1\]; \[1.5; −0.5\]\]_

### Statistics

mean(5; 6; 4; 2; 3; 7) _= 4.5_

stdev(5; 6; 4; 2; 3; 7) _≈ 1.87_

quartile((5; 6; 4; 2; 3; 7); 1) _= percentile(\[5; 6; 4; 2; 3; 7\]; 25) ≈ 2.9166667_

normdist(7; 5) _≈ 0.053990967_

spearman(column(load(test.csv); 1); column(load(test.csv); 2)) _≈ −0.33737388 (depends on the data in the CSV file)_

### Time and date

10:31 + 8:30 to time _= 19:01_

10h 31min + 8h 30min to time _= 19:01_

now to utc _= "2020-07-10T07:50:40Z"_

"2020-07-10T07:50CET" to utc+8 _= "2020-07-10T14:50:00+08:00"_

"2020-05-20" + 523d _= addDays(2020-05-20; 523) = "2021-10-25"_

today − 5 days _= "2020-07-05"_

"2020-10-05" − today _= days(today; 2020-10-05) = 87 d_

timestamp(2020-05-20) _= 1 589 925 600_

stamptodate(1 589 925 600) _= "2020-05-20T00:00:00"_

"2020-05-20" to calendars _(returns date in Hebrew, Islamic, Persian, Indian, Chinese, Julian, Coptic, and Ethiopian calendars)_

### Number bases

52 to bin _= 0011 0100_

52 to bin16 _= 0000 0000 0011 0100_

52 to oct _= 064_

52 to hex _= 0x34_

0x34 = hex(34) _= base(34; 16) = 52_

523<<2&250 to bin _= 0010 1000_

52.345 to float _≈ 0100 0010 0101 0001 0110 0001 0100 1000_

float(01000010010100010110000101001000) _= 1715241/32768 ≈ 52.345001_

flaotError(52.345) _≈ 1.2207031e-6_

52.34 to sexa _= 52°20′24″_

1978 to roman _= MCMLXXVIII_

52 to base 32 _= 1K_

sqrt(32) to base sqrt(2) _≈ 100000_

0xD8 to unicode _= Ø_

code(Ø) to hex _= 0xD8_

