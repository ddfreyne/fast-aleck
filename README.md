# Fast Aleck [![Build Status](https://secure.travis-ci.org/ddfreyne/fast-aleck.png)](http://travis-ci.org/ddfreyne/fast-aleck)

Fast Aleck is a tool for making text look smarter by using proper Unicode characters. For example, ... becomes … (`&hellip;`) and -- becomes — (`&mdash;`). Its goal is to be the fastest text cleaning tool out there. It is HTML-aware and also supports UTF-8.

Status
------

Fast Aleck is **alpha quality**. Expect things to break. If it breaks, please do let me know! Crash reports and examples of incorrect output are *greatly* appreciated!

How fast is it really?
----------------------

Here are the results of a benchmark in which a copy of Alice's Adventures in Wonderland is typographically enhanced 5000 times (see ruby/bench.rb):

	                                   user     system      total        real
	alice-rubypants (x10)         14.620000   0.470000  15.090000 ( 15.088082)
	alice-typogruby (x100)        32.820000   0.120000  32.940000 ( 32.957644)
	alice-fast-aleck-all-options  36.110000   5.040000  41.150000 ( 42.310613)
	alice-fast-aleck-no-options   12.880000   1.940000  14.820000 ( 14.932533)

`no-options` has the same features as Rubypants, while `all-options` has the same features as Typogruby. Rubypants is executed 10x less, and Typogruby is executed 100x less. These benchmarks indicate that Fast Aleck in its current state is about one order of magnitude faster than Rubypants, and about two orders of magnitude faster than Typogruby.

No development time has been spent on optimising Fast Aleck yet, but this will hopefully happen Soon™.

Features
--------

* ... → …
* -- → —
* --- → —
* '' → ‘’
* "" → “”
* Skips text inside `code`, `kbd`, `pre` and `script` elements
* Optionally wraps `&amp;` in `<span class="amp">` so you can pick the font with the prettiest ampersand
* Optionally wraps `'` in a span with class `quo`
* Optionally wraps `"` in a span with class `dquo`
* Optionally replaces last significant whitespace in a block with a non-breaking space (Widon’t)
* Optionally wraps sequences of capitals in `<span class="caps">` so you can use small caps.

Example input:

	This... CANNOT be true--it's impossible! "I had five EC2 instances!"

Example output:

	This… <span class="caps">CANNOT</span> be true&mdash;it’s impossible! “I had
	five <span class="caps">EC2</span>&nbsp;instances!”

Installation
------------

Building Fast Aleck requires CMake. Once you have that:

	% mkdir build
	% cd build
	% cmake ..
	% make

There will be three executables in the build directory once that is finished:

* `fast-aleck` is the commandline tool (try `fast-aleck --help` for help)
* `libfast-aleck.so` is the shared library (the exact name might differ depending on the OS)
* `fast-aleck-test` is a tool that runs the tests

If you intend to use the Ruby bindings, copy `libfast-aleck.so` to a public location (such as `/usr/local/lib`) or into the directory where you are executing Ruby from. For instance, if you want to use Fast Aleck with nanoc, you can copy `libfast-aleck.so` into the site directory (not in `./lib`).

Related Projects
----------------

* [fast-aleck-ruby](https://github.com/ddfreyne/fast-aleck-ruby) - Ruby bindings for Fast Aleck
* [fast-aleck-hs](https://github.com/jaspervdj/fast-aleck-hs) - Haskell bindings for Fast Aleck
