CXXFLAGS     = -std=c++14 -g -fopenmp -Ofast
OBJDIR       = obj
DEPDIR       = $(OBJDIR)/.deps
# Flags which, when added to gcc/g++, will auto-generate dependency files
DEPFLAGS     = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Function which takes a list of words and returns a list of unique words in that list
# https://stackoverflow.com/questions/16144115/makefile-remove-duplicate-words-without-sorting
uniq         = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# Source files - add more to auto-compile into .o files
SOURCES      = Common/gauss.cpp Part1-Bayes/main.cpp Part2-Euclid/main.cpp
# Executable targets - add more to auto-make in default 'all' target
EXEC         = Part1-Bayes/classify-bayes Part2-Euclid/classify-euclid
# Targets required for the homework, spearated by part
REQUIRED_1   = 
REQUIRED_2   = 
REQUIRED_OUT = $(REQUIRED_1) $(REQUIRED_2)

SOURCEDIRS   = $(call uniq, $(dir $(SOURCES)))
OBJDIRS      = $(addprefix $(OBJDIR)/, $(SOURCEDIRS))
DEPDIRS      = $(addprefix $(DEPDIR)/, $(SOURCEDIRS))
DEPFILES     = $(SOURCES:%.cpp=$(DEPDIR)/%.d)

.PHONY: all clean report

# By default, make all executable targets and the outputs required for the homework
all: $(EXEC) $(REQUIRED_OUT)

# Executable Targets
Part1-Bayes/classify-bayes: $(OBJDIR)/Part1-Bayes/main.o $(OBJDIR)/Common/gauss.o
	$(CXX) $(CXXFLAGS) $^ -o $@

Part2-Euclid/classify-euclid: $(OBJDIR)/Part2-Euclid/main.o $(OBJDIR)/Common/gauss.o
	$(CXX) $(CXXFLAGS) $^ -o $@

### Part 1 Outputs ###


### Part 2 Outputs ###


# Figures needed for the report
report: 

clean:
	rm -rf $(OBJDIR)
	rm -f $(EXEC)
	rm -rf out

# Generate .png images from .pgm images. Needed for report, since pdfLaTeX doesn't support .pgm images
%.png: %.pgm
	pnmtopng $< > $@

# Auto-Build .cpp files into .o
$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIRS) $(OBJDIRS)
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@

# Make generated directories
$(DEPDIRS) $(OBJDIRS) out: ; @mkdir -p $@
$(DEPFILES):
include $(wildcard $(DEPFILES))
