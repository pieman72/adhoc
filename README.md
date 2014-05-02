Action-Driven Human-Oriented Compiler
=====================================

I. Intro
--------

ADHOC is an application for abstracting programming logic away from
the syntax of any  particular language. The idea is that there is
some *important logic* that only a programmer can supply, but most
other program content is *syntactic fluff* for helping a text-based
compiler understand the important logic. ADHOC supplies a graphical
interface and optimization tools in order to let the programmer
focus solely on designing the important logic.


II. Package Management
----------------------

**[Installing]**

To install ADHOC, first create a directory to unpack the Git repo,
then use Git to clone the files, and finally use the included `make`
recipes to configure and install the application (you will need root
access):

	mkdir ~/adhoc_install; cd ~/adhoc_install
	git clone https://github.com/pieman72/adhoc .
	make configure install clean

*Note:* You will be asked for directories for ADHOC's binary,
library, and include files.


**[Updating]**

You can update ADHOC from the master branch and refresh your
installation by using the included `make` recipes:

	cd ~/adhoc_install; make update fresh

**[Uninstalling]**

To remove all installed ADHOC files, you can use the included make
recipe. You may then remove the repository itself:

	cd ~/adhoc_install; make uninstall clean
	cd ~; rm -rf adhoc_install


III. Usage
----------

**[Building a Logic File]**

ADHOC's "front-end" provides a graphical interface for expressing
program logic. It has not yet been inplemented... but I'm on it!

**[Generating Code]**

ADHOC's "back-end" reads in a logic file ('.adh') and outputs code
in a target language. Let's look at a basic example of how that's
done, assuming you've created a logic file called 'test.adh', and
you want to generate C code from that logic. 

	adhoc -l c -o test.c -e test.adh

Here's what this is doing:

* `-l c` *(lowercase 'L')* Sets the target language to C
* `-o test.c` Directs the generated code into the file 'test.c' instead of to `stdout`
* `-e` Makes the generated code ready as a standalone, including a `main()` function etc.
* `test.adh` the final argument should be the logic file being parsed

For a more complete list of CLI flags and options, run `adhoc -h`,
or see the '[Features](https://github.com/pieman72/adhoc#iv-features)'
section below.

**[Compiling Generated Code]**

For the most part, ADHOC attempts to make the generated code as
portable as possible. However some adhoc features are not available
in every language. To help, ADHOC comes with a number of support
libraries for different languages. Compilation instructions vary
from one language to the next. For example with C, ADHOC provides
a `libadhoc` library as part of the C-generation module. Here's how
you would link that library with the 'test.c' file mentioned above:

	gcc -o test test.c -L/usr/lib/adhoc -ladhoc

This creates an executable file called 'test' that has linked into it
any necessary features from `libadhoc`. *Note:* this assumes your
`ADHOC_LIB_PATH` was set to '/usr/lib' during configuration.

IV. Features
------------
**[Application Components]**

* **The 'adhoc.ini' config file** This stores options for how ADHOC runs,
	including the locations of the application's other files. This file
	itself is placed in the `ADHOC_LIB_PATH` (usually
	`/usr/lib/adhoc/adhoc.ini`), but you can override it with a local
	copy in your home directory or in the current working directory.
* **The 'adhoc' binary file** This is the main parser/generator that reads
	in a logic file and produces output. This is placed in the
	`ADHOC_BIN_PATH` directory.
* **The 'libadhoc.a' library** This is an archived library file that
 	mostly contains items for compiling C/C++ programs generated by ADHOC.
	It is placed in the `ADHOC_LIB_PATH` directory.
* **The 'libadhoc.h' header** This file further assists in compilation
	of C/C++ programs generated by ADHOC. It is stored in the
	`ADHOC_INC_PATH` directory.

**[CLI Arguments]**

* `-c filename, --config=filename`
	Use filename as ADHOC's configuration file instead of the
	default adhoc.ini file.
* `-d, --debug`
	Print out debug information while parsing the file.
* `-e, --executable`
	In addition to generating the target language code from the
	input logic, ADHOC will also include code necessary to execute
	the output program (e.g. when generating C code, it will include
	a 'main()' function).
* `-h, --help`
	Print this usage information.
* `-l lang, --language=lang`
	Set the target language for code generation to lang. This
	overrides the value set for `ADHOC_TARGET_LANGUAGE` in the config
	file.
* `-o filename, --outfile=filename`
	Directs generated target code to filename instead of stdout.
	Similar to adhoc ... > filename, but won't affect version info,
	etc.
* `-v, --version`
	Print ADHOC version information.


V. More Info
------------
**[Open Source]**

ADHOC is an open source project under the
[GPL v3](https://github.com/pieman72/adhoc/blob/master/LICENSE). You are
encouraged to get involved in, share, and contribute to the project.

**[Questions / Contact]**

Please [report any bugs](https://github.com/pieman72/adhoc/issues) you
find with ADHOC. If you have non-bug-related questions or comments, feel
free to email [adhoc@harveyserv.ath.cx](mailto:adhoc@harveyserv.ath.cx).
