NAME_CLI	:= cassave
NAME_GUI	:= cassie
VERSION		?= dev

CC		:= gcc
CFLAGS		:= -Wall 
CLDFLAGS	:= -lm -lsndfile
GUIFLAGS	:= $(shell pkg-config --cflags gtk4)
GUILDFLAGS	:= $(shell pkg-config --libs gtk4) -lsndfile -lm


BUILD		:= bin
RELEASE		:= release

SRC_CLI		:= src/main.c src/utils.c
SRC_GUI		:= src/gui.c src/utils.c

BIN_CLI		:= $(BUILD)/$(NAME_CLI)
BIN_GUI		:= $(BUILD)/$(NAME_GUI)

.PHONY: all clean release release-cli release-gui dist-cli dist-gui tar-cli tar-gui 7z-cli 7z-gui

#------------------------------------------------------------
#default build only cli

all: $(BIN_CLI)

$(BIN_CLI): $(SRC_CLI) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC_CLI) $(CLDFLAGS)
	strip $@

#$(BIN_GUI): $(SRC_GUI) | $(BUILD)
#	$(CC) $(GUIFLAGS) -o $@ $(SRC_GUI) $(GUILDFLAGS)
#	strip $@

$(BUILD):
	mkdir -p $@

#-------------------------------------------------------------
#clean
clean:
	rm -rf  $(build) $(RELEASE)

#------------------------------------------------------------
#full release

release: clean all release-cli release-gui
	@echo "Releases for $(VERSION) ready in $(RELEASE)/"



release-cli: dist-cli tar-cli 7z-cli
	@echo "CLI release $(VERSION) ready in $(RELEASE)/"

dist-cli: $(BIN_CLI)
	mkdir -p $(RELEASE)/$(NAME_CLI)-$(VERSION)/
	cp $(BIN_CLI) $(RELEASE)/$(NAME_CLI)-$(VERSION)/
	cp README.MD COPYING $(RELEASE)/$(NAME_CLI)-$(VERSION)/
	echo $(VERSION) > ./text/VERSION
	cp -r ./text/ $(RELEASE)/$(NAME_CLI)-$(VERSION)/

tar-cli:
	tar -C $(RELEASE) -czf $(RELEASE)/$(NAME_CLI)-$(VERSION).tar.gz $(NAME_CLI)-$(VERSION)

7z-cli:
	7z a $(RELEASE)/$(NAME_CLI)-$(VERSION).7z $(RELEASE)/$(NAME_CLI)-$(VERSION)



#release-gui: dist-gui tar-gui 7z-gui
#	@echo "GUI release $(VERSION) ready in $(RELEASE)/"

#dist-gui: $(BIN_GUI)
#	mkdir -p $(RELEASE)/$(NAME_GUI)-$(VERSION)
#	cp $(BIN_GUI) $(BIN_CLI) $(RELEASE)/$(NAME_GUI)-$(VERSION)/
#	cp README.MD COPYING $(RELEASE)/$(NAME_GUI)-$(VERSION)/
#	echo $(VERSION) > ./text/VERSION
#	cp -r ./text/ $(RELEASE)/$(NAME_GUI)-$(VERSION)/

#tar-gui:
#	tar -C $(RELEASE) -czf $(RELEASE)/$(NAME_GUI)-$(VERSION).tar.gz $(NAME_GUI)-$(VERSION)

#7z-gui:
#	7z a $(RELEASE)/$(NAME_GUI)-$(VERSION).7z $(RELEASE)/$(NAME_GUI)-$(VERSION)

