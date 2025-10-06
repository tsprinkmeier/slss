SHELL      = /bin/sh

MACH      ?= $(shell uname --machine)
APP        = slss
XTRA       = aont gfm
MDs        = $(wildcard *.md)
HTMLs      = $(MDs:.md=.html)
PDFs       = $(HTMLs:.html=.pdf)
DOC        = $(HTMLs) $(PDFs)

GIT_TAG    = $(APP)-$(shell git describe --tags --dirty --long)

CXXFLAGS  += -Wall -Wextra -Werror
CXXFLAGS  += -std=c++17
CXXFLAGS  += -ffile-prefix-map=$(CURDIR)=.
CXXFLAGS  += -ffile-prefix-map=$(abspath $(CURDIR))=.
CXXFLAGS  += '-DGIT_TAG="$(GIT_TAG)"'

ifdef DEBUG
CXXFLAGS  += -g
else
CXXFLAGS  += -O3
endif

CXXFLAGS += $(shell pkg-config --cflags openssl)
LDLIBS   += $(shell pkg-config --libs   openssl)

.PHONY: default
default: $(APP) doc

.PHONY: doc
doc: $(DOC)

.PHONY: xtra
xtra: $(XTRA)

.PHONY: clean
clean:
	-rm *.o
	-rm -rf .deps
	-rm $(DOC)

.PHONY: clobber cleaner
clobber cleaner: clean
	-rm $(APP) $(XTRA)

.PHONY: remake
remake: cleaner
	$(MAKE)

%.html: %.md
	markdown $^ > $@

%.pdf: %.html
	html2ps < $^ | ps2pdf - > $@

%.1.gz: %.1
	gzip --keep --force -- $^

blob.o::
	test -r $(MACH).objcopy
	git fsck
	git gc --aggressive
	git clone . $(GIT_TAG)
	git diff > $(GIT_TAG).diff
	tar --create --file - --sort=name --remove-files \
		$(GIT_TAG).diff $(GIT_TAG) \
		| xz > $(APP).tar.xz
	tar --format=v7 \
		--create --file $(APP).tar $(APP).tar.xz
	objcopy @$(MACH).objcopy $@
	rm $(APP).tar $(APP).tar.xz

$(APP): $(APP).o aont.o blob.o
	$(LINK.cc) -MMD $^ $(LOADLIBES) $(LDLIBS) -o $@

$(XTRA): $(APP)
	ln --force -- $^ $@

%.o: %.cc
	mkdir --parents .deps
	$(COMPILE.cc) -MMD $(OUTPUT_OPTION) $<
	@mv  $*.d  .deps/$*.d
	@echo $@: Makefile >> .deps/$*.d

.PHONY: install
install: $(APP) $(XTRA) $(APP).1.gz
	install --mode 0755 $(APP) $(XTRA) ~/.local/bin
	install --mode 0644 $(APP).1.gz    ~/.local/share/man/man1

-include .deps/*.d

.PHONY: apt
apt:
	sudo apt install build-essential g++ gcc libssl-dev markdown pkg-config html2ps
