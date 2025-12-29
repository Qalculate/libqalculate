# libqalculate
Qalculate! library and CLI

![Image of qalc](http://qalculate.github.io/images/qalc.png)

Qalculate! is a multi-purpose cross-platform desktop calculator. It is simple to use but provides power and versatility normally reserved for complicated math packages, as well as useful tools for everyday needs (such as currency conversion and percent calculation). Features include a large library of customizable functions, unit calculations and conversion, symbolic calculations (including integrals and equations), arbitrary precision, uncertainty propagation, interval arithmetic, plotting, and a user-friendly interface (GTK+, Qt, and CLI).

> [!TIP]
> Visit the website at https://qalculate.github.io/ to see additional screenshots, read the manual, and download the latest version.

## Requirements
* GMP and MPFR
* libxml2
* libcurl, icu, gettext (recommended)
* iconv, readline (recommended for CLI)
* Gnuplot (optional)
* doxygen (optional)

For Linux distributions which provide separate development packages, these must be installed for all the required libraries (e.g. libmpfr-dev) before compilation.

## Installation
Instructions and download links for installers, binaries packages, and the source code of released versions of Qalculate! are available at https://qalculate.github.io/downloads.html.

In a terminal window in the top source code directory run
* `./autogen.sh` *(not required if using a release source tarball, only if using the git version)*
* `./configure`
* `make`
* `make install` *(as root, e.g. `sudo make install`)*
* `ldconfig` *(if necessary, as root)*

If libqalculate is installed in /usr/local (default) you may need to add /usr/local/lib to the library path of the system (add /usr/local/lib to a file under /etc/ld.so.conf.d/ and run ldconfig).

## API Documentation
The API documentation is included in the package and is installed in $docdir/libqalculate/html (usually /usr/share/doc/libqalculate/html). It is generated when running autogen.sh.

It is also available online at http://qalculate.github.io/reference/index.html.

## Using the CLI program 'qalc'
To calculate a single expression from the command line (non-interactive mode) enter
`qalc mathematical expression` *(e.g. qalc 5+2)*

`qalc --help` shows information about command line options in non-interactive mode.

If you run `qalc` without any mathematical expression the program will start in interactive mode, where you can enter multiple expressions with history and completion, manipulate the result and change settings. Type `help` in interactive mode for more information.

A man page is also available (shown using the command `man qalc`, or online at https://qalculate.github.io/manual/qalc.html).

## Other Applications
The main user interfaces for libqalculate are
qalculate-gtk (https://github.com/Qalculate/qalculate-gtk) and
qalculate-qt (https://github.com/Qalculate/qalculate-qt).

Other software using libqalculate include
* Qalculator web app (https://qalculator.xyz/) (also available as an app for [Android](https://play.google.com/store/apps/details?id=xyz.qalculator.twa), [iOS](https://apps.apple.com/app/qalculator-xyz/id1611421527), and [Windows](https://www.microsoft.com/store/productId/9P4866X24PD3))
* Qalculate for Android (https://github.com/jherkenhoff/qalculate-android)
* KDE Plasma Workspace (https://www.kde.org/workspaces/plasmadesktop/)
* Cantor (http://kde.org/applications/education/cantor/)
* Step (http://kde.org/applications/education/step/)
* Qalculate widget for KDE Plasma (https://store.kde.org/p/1155946/)

## Features

#### General
   * All common operators — arithmetic, logical, bitwise, element-wise, (in)equalities
   * Expressions may contain any combination of numbers, constants, functions, units, variables, matrices, vectors, and time/dates
   * Fault-tolerant and flexible input parsing with verbose error/warning messages
   * Calculate as you type
   * Arbitrary precision with both rational and floating point numbers
   * Complex and infinite numbers
   * Propagation of uncertainty and interval arithmetic
   * Both exact and approximate output (*sqrt(32)* = *4 × sqrt(2)* ≈ *5.657*)
   * Simple and mixed fractions (*4 / 6 × 2* = *1.333…* = *4/3* = *1 + 1/3*)
   * All common number bases, as well as negative and non-integer radices, roman numerals, etc.
   * RPN mode
   * Highly customizable with a myriad of options for every aspect of the calculator

#### Symbolic calculations
   * Factorization and simplification
   * Differentiation and integration
   * Can solve most equations and inequalities
   * Customizable assumptions (e.g. *ln(2x) where x > 0* = *ln(2) + ln(x)*)

#### Functions
   * Over 400 flexible and diverse functions (trigonometry, exponents and logarithms, combinatorics, geometry, calculus, statistics, finance, time and date, etc.)
   * Easily created and edited from the user interfaces, with support for different argument types, subfunctions, and custom conditions

#### Units
   * Over 400 diverse units and all standard decimal and binary prefixes
   * Includes all SI units, as well as imperial, CGS, and atomic units, and more…
   * Automatic and explicit conversion (e.g. *ft + yd + m* = *2.2192 m*, *5 kg × m/s^2* = *5 N*, *5 m/s to mph* = *11.18 mph*)
   * Currency conversion with daily updated exchange rates
   * Easily created and edited from the user interfaces

#### Variables and constants
   * All common basic constants (π, e, φ, etc.)
   * Fundamental physical constants with 2022 CODATA values (including standard uncertainty and optional units)
   * CSV file import and export
   * Easily created and edited from the user interfaces (including by using simple assignment expressions, e.g. *x = 2 s*)
   * Data sets with objects and associated properties (a data set with chemical elements is included)

#### Plots and graphs
   * Uses Gnuplot
   * Can plot functions or data (matrices and vectors)
   * Can be customized using several options, and saved as image files

*…and more…*

> [!TIP]
> For more details about the syntax, and available functions, units, and variables, please consult the manual (https://qalculate.github.io/manual/).

## Examples (expressions)

> [!NOTE]
> Semicolon can be replaced with comma in function arguments, if comma is not used as decimal or thousands separator.

#### Basic functions and operators

```R
sqrt 4                        # = sqrt(4) = 4^(0.5) = 4^(1/2) = 2

sqrt(25; 16; 9; 4)            # = [5  4  3  2]

sqrt(32)                      # = 4 × √(2) (in exact mode)

cbrt(-27)                     # = root(-27; 3) = −3 (real root)

(-27)^(1/3)                   # ≈ 1.5 + 2.5980762i (principal root)

ln 25                         # = log(25; e) ≈ 3.2188758

log2(4)/log10(100)            # = log(4; 2)/log(100; 10) = 1

5!                            # = 1 × 2 × 3 × 4 × 5 = 120

5\2                           # = 5//2 = trunc(5 / 2) = 2 (integer division)

5 mod 3                       # = mod(5; 3) = 2

52 to factors                 # = 2^2 × 13

25/4 * 3/5 to fraction        # = 3 + 3/4

gcd(63; 27)                   # = 9

sin(pi/2) - cos(pi)           # = sin(90 deg) − cos(180 deg) = 2

sum(x; 1; 5)                  # = 1 + 2 + 3 + 4 + 5 = 15

sum(\i^2+sin(\i); 1; 5; \i)   # = 1^2 + sin(1) + 2^2 + sin(2) + … ≈ 55.176162

product(x; 1; 5)              # = 1 × 2 × 3 × 4 × 5 = 120
 
var1:=5                       # stores value 5 in variable var1
var1 * 2                      # = 10

sinh(0.5) where sinh()=cosh() # = cosh(0.5) ≈ 1.1276260

plot(x^2; -5; 5)              # plots the function y=x^2 from -5 to 5
```

#### Units

> [!TIP]
> ` to ` can be replace with an arrow (`➞` or `->`).

```R
5 dm3 to l                    # = 5 dm^3 to L = 5 L

20 miles / 2 h to km/h        # = 16.09344 km/h

1.74 to ft                    # = 1.74 m to ft ≈ 5 ft + 8.5039370 in

1.74 m to -ft                 # ≈ 5.7086614 ft

100 lbf * 60 mph to hp        # ≈ 16 hp

50 Ω * 2 A                    # = 100 V

50 Ω * 2 A to base            # = 100 kg·m²/(s³·A)

10 N / 5 Pa                   # = (10 N)/(5 Pa) = 2 m²

5 m/s to s/m                  # = 0.2 s/m

€500 - 20% to £               # ≈ £347.12

500 megabit/s * 2 h to b?byte # ≈ 419.09516 gibibytes
```

#### Physical constants

```bash
k_e / G * a_0
# = (CoulombsConstant / NewtonianConstant) × BohrRadius ≈ 7.126e9 kg·H·m^−1

ℎ / (λ_C * c)
# = planck ∕ (ComptonWavelength × SpeedOfLight) ≈ 9.1093837e-31 kg

5 ns * rydberg to c
# ≈ 6.0793194E-8c

atom(Hg; weight) + atom(C; weight) * 4 to g
# ≈ 4.129e-22 g

(G * planet(earth; mass) * planet(mars; mass))/(54.6e6 km)^2
# ≈ 8.58e16 N (gravitational attraction between earth and mars)
```

#### Uncertainty and interval arithmetic

> [!NOTE]
> Results with interval arithmetic activated are shown in parenthesis.

```R
sin(5±0.2)^2/2±0.3   # ≈ 0.460±0.088 (0.46±0.12)

(2±0.02 J)/(523±5 W) # ≈ 3.824±0.053 ms (3.825±0.075 ms)

interval(-2; 5)^2    # ≈ interval(−8.2500000; 12.750000) (interval(0; 25))
```

> [!TIP]
> "±" can be replaced with "+/-".

#### Algebra

```ruby
(5x^2 + 2)/(x - 3)                            # = 5x + 15 + 47/(x − 3)

(\a + \b)(\a - \b)                            # = 'a'^2 − 'b'^2

(x + 2)(x - 3)^3                              # = x^4 − 7x^3 + 9x^2 + 27x − 54

x^4 - 7x^3 + 9x^2 + 27x - 54 to factors       # = (x + 2)(x − 3)^3

cos(x)+3y^2 where x=pi; y=2                   # = 11

gcd(25x; 5x^2)                                # = 5x

1/(x^2+2x-3) to partial fraction              # = 1/(4x − 4) − 1/(4x + 12)

x+x^2+4 = 16                                  # x = 3 or x = −4

x^2/(5 m) - hypot(x; 4 m) = 2 m where x > 0   # x ≈ 7.1340411 m

cylinder(20cm; x) = 20l                       # x = (1 / (2π)) m ≈ 16 cm

asin(sqrt(x)) = 0.2                           # x = sin(0.2)^2 ≈ 0.039469503

x^2 > 25x                                     # x > 25 or x < 0

solve(x = y+ln(y); y)                         # = lambertw(e^x)

solve2(5x=2y^2; sqrt(y)=2; x; y)              # = 32/5

multisolve([5x=2y+32, y=2z, z=2x]; [x, y, z]) # = [−32/3  −128/3  −64/3]

dsolve(diff(y; x) - 2y = 4x; 5)               # = 6e^(2x) − 2x − 1
```

#### Calculus

```ruby
diff(6x^2)                                    # = 12x

diff(sinh(x^2)/(5x) + 3xy/sqrt(x))            # = (2/5) × cosh(x^2) − sinh(x^2)/(5x^2) + (3y)/(2 × √(x))

integrate(6x^2)                               # = 2x^3 + C

integrate(6x^2; 1; 5)                         # = 248

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x))       # = 2x × √(x) × y + Shi(x^2) / 10 + C

integrate(sinh(x^2)/(5x) + 3xy/sqrt(x); 1; 2) # ≈ 3.6568542y + 0.87600760

limit(ln(1 + 4x)/(3^x - 1); 0)                # = 4 / ln(3)
```

#### Matrices and vectors

```R
[1, 2, 3; 4, 5, 6]                      # = [1  2  3; 4  5  6] (2×3 matrix)

1…5 = (1:5) = (1:1:5)                   # = [1  2  3  4  5]

(1; 2; 3) * 2 - 2                       # = [(1 × 2 − 2), (2 × 2 − 2), (3 × 2 − 2)] = [0  2  4]

[1 2 3].[4 5 6] = dot([1 2 3]; [4 5 6]) # = 32 (dot product)

cross([1 2 3]; [4 5 6])                 # = [−3 6 −3] (cross product)

[1 2 3; 4 5 6].*[7 8 9; 10 11 12]       # = [7  16  27; 40  55  72] (hadamard product)

[1 2 3; 4 5 6] * [7 8; 9 10; 11 12]     # = [58  64; 139  154] (matrix multiplication)

[1 2; 3 4]^-1                           # = inverse([1 2; 3 4]) = [−2  1; 1.5  −0.5]
```

#### Statistics

```R
mean(5; 6; 4; 2; 3; 7)             # = 4.5

stdev(5; 6; 4; 2; 3; 7)            # ≈ 1.87

quartile([5 6 4 2 3 7]; 1)         # = percentile((5; 6; 4; 2; 3; 7); 25) ≈ 2.9166667

normdist(7; 5)                     # ≈ 0.053990967

spearman(column(load(test.csv); 1); column(load(test.csv); 2))
# ≈ −0.33737388 (depends on the data in the CSV file)
```

#### Time and date

```ruby
10:31 + 8:30 to time           # = 19:01

10h 31min + 8h 30min to time   # = 19:01

now to utc                     # = "2020-07-10T07:50:40Z"

"2020-07-10T07:50CET" to utc+8 # = "2020-07-10T14:50:00+08:00"

"2020-05-20" + 523d            # = addDays(2020-05-20; 523) = "2021-10-25"

today - 5 days                 # = "2020-07-05"

"2020-10-05" - today           # = days(today; 2020-10-05) = 87 d

timestamp(2020-05-20)          # = 1 589 925 600

stamptodate(1 589 925 600)     # = "2020-05-20T00:00:00"

"2020-05-20" to calendars
# returns date in Hebrew, Islamic, Persian, Indian, Chinese, Julian, Coptic, and Ethiopian calendars
```

#### Number bases

```R
52 to bin                               # = 0011 0100

52 to bin16                             # = 0000 0000 0011 0100

52 to oct                               # = 064

52 to hex                               # = 0x34

0x34 = hex(34)                          # = base(34; 16) = 52

523<<2&250 to bin                       # = 0010 1000

52.345 to float                         # ≈ 0100 0010 0101 0001 0110 0001 0100 1000

float(01000010010100010110000101001000) # = 1715241/32768 ≈ 52.345001

floatError(52.345)                      # ≈ 1.2207031e-6

52.34 to sexa                           # = 52°20′24″

1978 to roman                           # = MCMLXXVIII

52 to base 32                           # = 1K

sqrt(32) to base sqrt(2)                # ≈ 100000

0xD8 to unicode                         # = Ø

code(Ø) to hex                          # = 0xD8
```
