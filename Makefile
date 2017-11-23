#===============================================================================
#		Makefile for muzz.c
#		Convert decimal numbers to binary
#===============================================================================

CC=gcc
PREFIX=/usr
FILES=muzz.c
#OPTFLAGS=-g -Wall
LDFLAGS=-lm
OPTFLAGS=-O3
OUTPUT=muzz
SRC=src
DOC=doc
OUTPUTDIR=$(PREFIX)/bin
DOCPATH=$(PREFIX)/share/doc/muzz
LICENSEPATH=$(PREFIX)/share/licenses/muzz

all: $(SRC)/muzz.c
	$(CC) $(LDFLAGS) $(OPTFLAGS) -o $(OUTPUT) $(SRC)/$(FILES)

install:
	install $(OUTPUT) -D $(OUTPUTDIR)/$(OUTPUT)
	install README -D $(DOCPATH)/README
	install $(DOC)/CHANGES -D $(DOCPATH)/CHANGES
	install $(DOC)/LICENSE -D $(LICENSEPATH)/LICENSE

uninstall:
	rm -f $(OUTPUTDIR)/$(OUTPUT)
	rm -r $(DOCPATH)
	rm -r $(LICENSEPATH)

clean:
	rm -f $(OUTPUT)
