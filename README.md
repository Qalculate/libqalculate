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
* `make install` (as root)
* `ldconfig` (if necessary, as root)
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
   * Basic operations and operators: + - * / ^ E () && || ! < > >= <= != ~ & | << >>
   * Fault-tolerant parsing of strings: log 5 / 2 .5 (3) + (2( 3 +5 = ln(5) / (2.5 * 3) + 2 * (3 + 5)
   * Expressions may contain any combination of numbers, functions, units, variables, vectors and matrices, and dates
   * Supports complex and infinite numbers
   * Propagation of uncertainty
   * Interval arithmetic (for determination of the number of significant digits or direct calculation with intervals of numbers)
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
   * Ability to disable functions, variables, units or unknown variables for less confusion: ex. when you do not want (a+b)^2 to mean (are+barn)^2 but ("a"+"b")^2
   * Controllable implicit multiplication
   * Matrices and vectors, and related operations (determinants etc.)
   * Verbose error messages
   * Arbitrary precision
   * RPN mode
* Result display:
   * Supports all common number bases, as well as negative and non-integer radices, sexagesimal numbers, time format, and roman numerals
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

## Examples

###Arithmetics

5^2 = 25

sqrt(4) = 4^(0.5) = 4^(1/2) = 2

sqrt(32) = 4 × √(2) _(in exact mode)_

cbrt(27) = root(27, 3) = 27^(1/3) = 3

cbrt(-27) = -3 _(real root)_

(-27)^(1/3) ≈ 1.5 + 2.5980762i _(principal root)_

log2(4) = log(4, 2) = 2

log10(100) = log(100, 10) = 2

ln(25) = log(25, e) ≈ 3.2188758

5! = 1 × 2 × 3 × 4 × 5 = 120

sum(x, 1, 5) = 1 + 2 + 3 + 4 + 5 = 15

sum(\i^2+sin(\i), 1, 5, \i) = 1^2 + sin(1) + 2^2 + sin(2) + ... ≈ 55.176162

product(x, 1, 5) = 1 × 2 × 3 × 4 × 5 = 120

###Units

20 miles / 2h to km/h = 16.09344 km/h

1.74 to ft = 1.74 m to ft ≈ 5 ft + 8.5039370 in

1.74 m to -ft ≈ 5.7086614 ft

5 J × 523 s × 15 mph = 17.535144 kJ·m

5 J × 523 s × 15 mph to base = 17.535144 Mg·m^3/s^2

5 m/s to s/m = 0.2 s/m

500 € to $ ≈ 566.25

###Physical constants

k_e / G × a_0 = (coulombs_constant / newtonian_constant) × bohr_radius ≈ 7.126e9 kg·H·m^−1

5 ns × rydberg to c ≈ 6.0793194E-8c

###Uncertainty and interval arithmetic

_"±" can be replaced with "+/-"_
_result with interval arithmetic activated is shown in parenthesis_

sin(5±0.2)^2/2±0.3 ≈ 0.460±0.088 _(0.46±0.12)_

(2±0.02 J)/(523±5 W) ≈ 3.824±0.053 ms _(3.825±0.075 ms)_

interval(−2, 5)^2 ≈ intervall(−8.2500000, 12.750000) _(intervall(0, 25))_

###Algebra

(5x^2 + 2)/(x − 3) = 5x + 15 + 47∕(x − 3)

(\a + \b)(\a − \b) = ("a" + "b")("a" − "b") = 'a'^2 − 'b'^2

(x + 2)(x − 3)^3 = x^4 − 7x^3 + 9x^2 + 27x − 54

factorize x^4 − 7x^3 + 9x^2 + 27x − 54 = x^4 − 7x^3 + 9x^2 + 27x − 54 to factors = (x + 2)(x − 3)^3

1/(x^2+2x−3) to partial fraction = 1/(4x − 4) − 1/(4x + 12)

x+x^2+4 = 16
	= x = 3 eller x = −4

cylinder(20cm, x) = 20L (calculates the height of a 20 L cylinder with radius of 20 cm)
	= x = (1 ∕ (2π)) m
	= x ≈ 16 cm

asin(sqrt(x)) = 0.2
	= x = sin(0.2)^2
	= x ≈ 0.039469503

solve2(5x=2y^2, sqrt(y)=2, x, y) = 32/5

multisolve(\[5x=2y+32, y=2z, z=2x\], \[x, y, z\]) = \[−32∕3, −128∕3, −64/3\]


###Calculus

diff(6x^2) = 12x

diff(sinh(x^2)/(5x) + 3xy/sqrt(x)) = (2∕5) × cosh(x^2) − sinh(x^2)∕(5x^2) + (3y)∕(2 × √(x))

integrate(6x^2) = 2x^3 + C

integrate(6x^2, 1, 5) = 248

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x)) = 2x × √(x) × y + Shi(x^2) ∕ 10 + C

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x), 1, 2) ≈ 3.6568542y + 0.87600760


###Time and date

10:31 + 8:30 to time = 19:01

10h 31min + 8h 30min to time = 19:01

now to utc = "2020-07-10T07:50:40Z"

"2020-07-10T07:50CET" to utc+8 = "2020-07-10T14:50:00+08:00"

"2020−05−20" + 523d = "2021-10-25"

addDays(2020-05-20, 523) = "2021-10-25"

today − 5 days = "2020-07-05"

"2020-10-05" − today = 87 d

days(today, 2020-10-05) = 87

timestamp(2020−05−20) = 1 589 925 600

stamptodate(1 589 925 600) = "2020−05−20T00:00:00"

"2020-05-20" to calendars (returns date in Hebrew, Islamic, Persian, Indian, Chinese, Julian, Coptic, and Ethiopian calendars)

Time units:
https://qalculate.github.io/manual/qalculate-definitions-units.html#qalculate-definitions-units-1-Time
Data and time functions:
https://qalculate.github.io/manual/qalculate-definitions-functions.html#qalculate-definitions-functions-1-Date--Time

###Number bases

52 to bin = 0011 0100

52 to bin16 = 0000 0000 0011 0100

52 to oct = 064

52 to hex = 0x34

0x34 = hex(34) = base(34, 16) = 52

523<<2&250 to bin = 0010 1000

52.345 to float ≈ 0100 0010 0101 0001 0110 0001 0100 1000

float(01000010010100010110000101001000) = 1715241/32768 ≈ 52.345001

flaotError(52.345) ≈ 1.2207031e-6

52.34 to sexa = 52°20′24″

1978 to roman = MCMLXXVIII

52 to base 32 = 1K

sqrt(32) to base sqrt(2) ≈ 100000

0xD8 to unicode = Ø

code(Ø) to hex = 0xD8


