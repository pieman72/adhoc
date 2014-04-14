CC=gcc
CFLAGS=-I.

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

.PHONY: test
test: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@./adhoc programs/test.adh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: grind_full
grind_full: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@valgrind -v --leak-check=full --show-reachable=yes ./adhoc programs/test.adh
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: grind
grind: adhoc
	@echo "$(LC4)-- Testing ADHOC --$(NORMAL)"
	@valgrind -v --leak-check=full --show-reachable=yes ./adhoc programs/test.adh 2>&1 | grep '\(LEAK SUMMARY\)\|\(All heap blocks were freed\)\|\(ERROR SUMMARY\)' | sed 's/\(0 errors from 0 contexts\|no leaks are possible\)/$(LC3)\1$(NORMAL)/'
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
	@echo "$(LC4)-- Commit Prepared, Pushing to GitHub --$(NORMAL)"
	@git push https://github.com/pieman72/adhoc master
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: update
update: clean
	@echo "$(LC1)-- Fetching Remote Changes --$(NORMAL)"
	@git pull https://github.com/pieman72/adhoc
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: diff
diff: clean
	@echo "$(LC1)-- Showing Local Diff --$(NORMAL)"
	@git diff | /usr/share/vim/vim72/macros/less.sh
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
	@rm -rf lex.yy.c y.tab.c y.tab.h adhoc
	@echo "[ $(LC3)OK$(NORMAL) ]\n"

.PHONY: clear
clear:
	@clear
