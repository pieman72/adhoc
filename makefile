CFLAGS=-I.
CC=gcc $(CFLAGS)

NORMAL=[0;39m
RED=[38;5;160m
ORANGE=[38;5;208m
YELLOW=[38;5;226m
GREEN=[38;5;40m
BLUE=[38;5;33m
INDIGO=[38;5;61m
VIOLET=[38;5;91m
LC1=[38;5;175m
LC2=[38;5;214m
LC3=[38;5;119m
LC4=[38;5;117m
LC5=[38;5;125m

.PHONY: fresh
fresh: uninstall install clean

.PHONY: configure
configure:
	@echo "$(LC2)-- Configuring ADHOC --$(NORMAL)"
	@if [ -z `which gcc` ]; then\
		echo "$(RED)Error:$(NORMAL) It seems you do not have access to 'gcc'. Please install it first, in order to install ADHOC.\n";\
		return 1;\
	fi
	@echo "Please choose a path for ADHOC binary files (here are some suggestions)"
	@echo " /usr/bin\n"
	@read -p "Binary path: " ADHOC_BIN_PATH;\
	ADHOC_BIN_PATH=`echo $$ADHOC_BIN_PATH | sed 's/\/$$//'`;\
	echo "\nPlease choose a path for ADHOC library files (here are some suggestions)";\
	echo " /usr/lib\n";\
	read -p "Library path: " ADHOC_LIB_PATH;\
	ADHOC_LIB_PATH=`echo $$ADHOC_LIB_PATH | sed 's/\/$$//'`;\
	echo "\nPlease choose a path for ADHOC include files (here are some suggestions)";\
	echo " /usr/include\n";\
	read -p "Include path: " ADHOC_INC_PATH;\
	ADHOC_INC_PATH=`echo $$ADHOC_INC_PATH | sed 's/\/$$//'`;\
	cat adhoc.ini\
		| grep -v '^ADHOC_\(BIN\|LIB\|INC\)_PATH='\
		| awk '/\[compiler\]/ {\
			print;\
			print "ADHOC_BIN_PATH='`echo $$ADHOC_BIN_PATH`'";\
			print "ADHOC_LIB_PATH='`echo $$ADHOC_LIB_PATH`'";\
			print "ADHOC_INC_PATH='`echo $$ADHOC_INC_PATH`'";\
			next }1' > adhoc.ini
	@echo "\n[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: install
install: adhoc modules
	@echo "$(LC4)-- Installing ADHOC --$(NORMAL)"
	@ADHOC_BIN_PATH=`cat adhoc.ini\
		| grep '^ADHOC_BIN_PATH='\
		| sed 's/ADHOC_BIN_PATH=//'`;\
	ADHOC_LIB_PATH=`cat adhoc.ini\
		| grep '^ADHOC_LIB_PATH='\
		| sed 's/ADHOC_LIB_PATH=//'`;\
	ADHOC_INC_PATH=`cat adhoc.ini\
		| grep '^ADHOC_INC_PATH='\
		| sed 's/ADHOC_INC_PATH=//'`;\
	if [ -d $$ADHOC_LIB_PATH'/adhoc' ]; then\
		echo "Library path exists. Will not touch it.";\
	else\
		echo 'Creating ADHOC library dir at: $(LC2)'$$ADHOC_LIB_PATH'/adhoc$(NORMAL)';\
		sudo mkdir $$ADHOC_LIB_PATH'/adhoc';\
	fi;\
	echo 'Copying ADHOC binary';\
	sudo install -D adhoc $$ADHOC_BIN_PATH'/adhoc';\
	echo 'Copying library files';\
	echo -n 'adhoc.ini\nlibadhoc.a'\
		| xargs -I% sh -c 'sudo cp % '$$ADHOC_LIB_PATH'/adhoc/';\
	echo 'Copying include files';\
	sudo cp libadhoc.h $$ADHOC_INC_PATH
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: uninstall
uninstall:
	@echo "$(LC4)-- Uninstalling Any Existing ADHOC Files --$(NORMAL)"
	@ADHOC_BIN_PATH=`cat adhoc.ini\
		| grep '^ADHOC_BIN_PATH='\
		| sed 's/ADHOC_BIN_PATH=//'`;\
	ADHOC_LIB_PATH=`cat adhoc.ini\
		| grep '^ADHOC_LIB_PATH='\
		| sed 's/ADHOC_LIB_PATH=//'`;\
	ADHOC_INC_PATH=`cat adhoc.ini\
		| grep '^ADHOC_INC_PATH='\
		| sed 's/ADHOC_INC_PATH=//'`;\
	sudo rm -rf %%ADHOC_BIN_PATH'/adhoc' $$ADHOC_LIB_PATH'/adhoc' $$ADHOC_INC_PATH'/libadhoc.h'
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: run
run: adhoc
	@echo "$(LC4)-- Running ADHOC --$(NORMAL)"
	@./adhoc programs/test.adh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: test
test: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@./adhoc -d -e programs/test.adh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: grind_full
grind_full: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@valgrind -v --leak-check=full --show-reachable=yes --track-origins=yes ./adhoc -e programs/test.adh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: grind
grind: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@valgrind -v --leak-check=full --show-reachable=yes --track-origins=yes ./adhoc -e programs/test.adh 2>&1 | grep '\(LEAK SUMMARY\)\|\(All heap blocks were freed\)\|\(ERROR SUMMARY\)' | sed 's/\(0 errors from 0 contexts\|no leaks are possible\)/$(LC3)\1$(NORMAL)/'
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: push
push: commit
	@echo "$(LC4)-- Commit Prepared, Pushing to GitHub --$(NORMAL)"
	@git push origin `git rev-parse --abbrev-ref HEAD`
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: commit
commit: clean
	@echo "$(LC1)-- Showing Modified Local Files --$(NORMAL)"
	@git status
	@echo "[ $(LC3)OK$(NORMAL) ]\n"
	@echo "$(LC2)-- Select Files To Commit --$(NORMAL)"
	@read -p "Files: " ADHOC_COMMIT_LIST; git add $$ADHOC_COMMIT_LIST
	@echo "[ $(LC3)OK$(NORMAL) ]\n"
	@echo "$(LC3)-- Files Chosen, Add Message --$(NORMAL)"
	@git commit
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: update
update: pull

.PHONY: pull
pull:
	@echo "$(LC1)-- Fetching Remote Changes --$(NORMAL)"
	@git pull origin
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: diff
diff: clean
	@echo "$(LC1)-- Showing Local Diff --$(NORMAL)"
	@git diff | /usr/share/vim/vim74/macros/less.sh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: merge
merge: clean
	@echo "$(LC1)-- Merging Current Branch Into Master --$(NORMAL)"
	@ADHOC_CURRENT_BRANCH=`git rev-parse --abbrev-ref HEAD`;\
	if [ $$ADHOC_CURRENT_BRANCH = 'master' ]; then\
		echo "Already working on $(LC1)master$(NORMAL)";\
		return 1;\
	fi;\
	git checkout master;\
	git merge $$ADHOC_CURRENT_BRANCH;\
	git push origin --delete $$ADHOC_CURRENT_BRANCH
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: branch
branch: clean
	@echo "$(LC1)-- Create a New Branch --$(NORMAL)"
	@echo 'Current branch:$(LC4)' `git rev-parse --abbrev-ref HEAD` '$(NORMAL)'\
		| sed 's/master/$(LC1)master/'
	@read -p "New branch name: " ADHOC_BRANCH_NAME;\
	git checkout -b $$ADHOC_BRANCH_NAME;\
	echo 'Now using: $(LC4)' $$ADHOC_BRANCH_NAME '$(NORMAL)'
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: modules
modules: c_module

.PHONY: c_module
c_module:
	@echo "$(LC1)-- Compiling C Module --$(NORMAL)"
	@$(CC) -Wall -fPIC -c libadhoc.c -o libadhoc.o
	@ar -cq libadhoc.a libadhoc.o
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: adhoc
adhoc: grammar lex.yy.c y.tab.c adhoc.h adhoc_types.h
	@echo "$(LC3)-- Creating Compiler --$(NORMAL)"
	@$(CC) lex.yy.c y.tab.c -o $@
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: grammar
grammar: adhoc.l adhoc.y
	@echo "$(LC1)-- Checking Lexemes --$(NORMAL)"
	@lex adhoc.l
	@echo "[ $(LC3)OK$(NORMAL) ]\n"
	@echo "$(LC2)-- Checking Grammar --$(NORMAL)"
	@yacc -d adhoc.y
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: clean
clean:
	@echo "$(LC5)-- Cleaning Up --$(NORMAL)"
	@rm -rf lex.yy.c y.tab.c y.tab.h adhoc libadhoc.a *.o
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: clear
clear:
	@clear
