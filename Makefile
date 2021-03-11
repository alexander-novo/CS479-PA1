CXXFLAGS     = -std=c++14 -g -fopenmp -O3
OBJDIR       = obj
DEPDIR       = $(OBJDIR)/.deps
# Flags which, when added to gcc/g++, will auto-generate dependency files
DEPFLAGS     = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Function which takes a list of words and returns a list of unique words in that list
# https://stackoverflow.com/questions/16144115/makefile-remove-duplicate-words-without-sorting
uniq         = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# Source files - add more to auto-compile into .o files
SOURCES      = Common/sample.cpp Part1-Bayes/main.cpp Part2-Euclid/main.cpp
INCLUDES     = -I include/
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

.PHONY: all exec clean report
.SECONDARY:

# By default, make all executable targets and the outputs required for the homework
all: exec $(REQUIRED_OUT) Report/report.pdf
exec: $(EXEC)

# Executable Targets
Part1-Bayes/classify-bayes: $(OBJDIR)/Part1-Bayes/main.o $(OBJDIR)/Common/sample.o
	$(CXX) $(CXXFLAGS) $^ -o $@

Part2-Euclid/classify-euclid: $(OBJDIR)/Part2-Euclid/main.o $(OBJDIR)/Common/sample.o
	$(CXX) $(CXXFLAGS) $^ -o $@

### Part 1 Outputs ###
out/bayes/sample1-%.dat out/bayes/sample2-%.dat out/bayes/sample1-misclass-%.dat out/bayes/sample2-misclass-%.dat out/bayes/error-bound-%.dat out/bayes/params-%.dat out/bayes/pdf-%.dat out/bayes/classification-rate-%.txt: Part1-Bayes/classify-bayes | out/bayes
	Part1-Bayes/classify-bayes $* -ps1 out/bayes/sample1-$*.dat\
	                              -ps2 out/bayes/sample2-$*.dat\
	                              -pm1 out/bayes/sample1-misclass-$*.dat\
	                              -pm2 out/bayes/sample2-misclass-$*.dat\
	                              -pdf out/bayes/pdf-$*.dat\
	                              -peb out/bayes/error-bound-$*.dat\
	                              -pdb out/bayes/params-$*.dat | tee out/bayes/classification-rate-$*.txt

out/bayes/sample-plot-%.png: out/bayes/sample1-%.dat out/bayes/sample2-%.dat out/bayes/params-%.dat  Part1-Bayes/plot.plt
	@gnuplot -e "outfile='$@'"\
	         -e "sample1='out/bayes/sample1-$*.dat'"\
	         -e "sample2='out/bayes/sample2-$*.dat'"\
	         -e "plotTitle='Data Set $*'"\
	         -e "paramFile='out/bayes/params-$*.dat'"\
	         Part1-Bayes/plot.plt

out/bayes/misclass-plot-%.png: out/bayes/sample1-misclass-%.dat out/bayes/sample2-misclass-%.dat out/bayes/params-%.dat Part1-Bayes/plot.plt
	@gnuplot -e "outfile='$@'"\
	         -e "sample1='out/bayes/sample1-misclass-$*.dat'"\
	         -e "sample2='out/bayes/sample2-misclass-$*.dat'"\
	         -e "plotTitle='Misclassified Observations From Data Set $*'"\
	         -e "paramFile='out/bayes/params-$*.dat'"\
	         Part1-Bayes/plot.plt

out/bayes/sample-pdf-plot-%.png: out/bayes/pdf-%.dat out/bayes/sample1-%.dat out/bayes/sample2-%.dat out/bayes/params-%.dat Part1-Bayes/plot-pdf.plt
	@gnuplot -e "outfile='$@'"\
	         -e "pdfFile='out/bayes/pdf-$*.dat"\
	         -e "plotTitle='Joint pdf and samples from Data Set $*'"\
	         -e "sample1='out/bayes/sample1-$*.dat'"\
             -e "sample2='out/bayes/sample2-$*.dat'"\
	         -e "paramFile='out/bayes/params-$*.dat'"\
	         Part1-Bayes/plot-pdf.plt

out/bayes/misclass-pdf-plot-%.png: out/bayes/pdf-%.dat out/bayes/sample1-misclass-%.dat out/bayes/sample2-misclass-%.dat out/bayes/params-%.dat Part1-Bayes/plot-pdf.plt
	@gnuplot -e "outfile='$@'"\
	         -e "pdfFile='out/bayes/pdf-$*.dat"\
	         -e "plotTitle='Joint pdf and misclassified samples from Data Set $*'"\
	         -e "sample1='out/bayes/sample1-misclass-$*.dat'"\
             -e "sample2='out/bayes/sample2-misclass-$*.dat'"\
	         -e "paramFile='out/bayes/params-$*.dat'"\
	         Part1-Bayes/plot-pdf.plt

out/bayes/error-bound-%.pdf: out/bayes/error-bound-%.dat out/bayes/params-%.dat Part1-Bayes/plot-error-bound.plt
	@gnuplot -e "outfile='$@'"\
	         -e "plotTitle='Error bound function for Data Set $*'"\
	         -e "boundFile='out/bayes/error-bound-$*.dat'"\
	         -e "paramFile='out/bayes/params-$*.dat'"\
	         Part1-Bayes/plot-error-bound.plt

### Part 2 Outputs ###
out/euclid/sample1-%.dat out/euclid/sample2-%.dat out/euclid/sample1-misclass-%.dat out/euclid/sample2-misclass-%.dat out/euclid/params-%.dat out/euclid/classification-rate-%.txt: Part2-Euclid/classify-euclid | out/euclid
	Part2-Euclid/classify-euclid $* -ps1 out/euclid/sample1-$*.dat\
	                              -ps2 out/euclid/sample2-$*.dat\
	                              -pm1 out/euclid/sample1-misclass-$*.dat\
	                              -pm2 out/euclid/sample2-misclass-$*.dat\
	                              -pdb out/euclid/params-$*.dat | tee out/euclid/classification-rate-$*.txt

out/euclid/sample-plot-%.png: out/euclid/sample1-%.dat out/euclid/sample2-%.dat out/euclid/params-%.dat Part1-Bayes/plot.plt
	@gnuplot -e "outfile='$@'"\
	         -e "sample1='out/euclid/sample1-$*.dat'"\
	         -e "sample2='out/euclid/sample2-$*.dat'"\
	         -e "plotTitle='Data Set $*'"\
	         -e "paramFile='out/euclid/params-$*.dat'"\
	         Part1-Bayes/plot.plt

out/euclid/misclass-plot-%.png: out/euclid/sample1-misclass-%.dat out/euclid/sample2-misclass-%.dat out/euclid/params-%.dat Part1-Bayes/plot.plt
	@gnuplot -e "outfile='$@'"\
	         -e "sample1='out/euclid/sample1-misclass-$*.dat'"\
	         -e "sample2='out/euclid/sample2-misclass-$*.dat'"\
	         -e "plotTitle='Misclassified Observations From Data Set $*'"\
	         -e "paramFile='out/euclid/params-$*.dat'"\
	         Part1-Bayes/plot.plt

# Figures needed for the report
# Part 1
report: out/bayes/sample-plot-A.png out/bayes/sample-plot-B.png out/bayes/misclass-plot-A.png out/bayes/misclass-plot-B.png
report: out/bayes/sample-pdf-plot-A.png out/bayes/sample-pdf-plot-B.png out/bayes/misclass-pdf-plot-A.png out/bayes/misclass-pdf-plot-B.png
report: out/bayes/classification-rate-A.txt out/bayes/classification-rate-B.txt out/bayes/error-bound-A.pdf out/bayes/error-bound-B.pdf
# Part 2
report: out/euclid/sample-plot-A.png out/euclid/sample-plot-B.png out/euclid/misclass-plot-A.png out/euclid/misclass-plot-B.png
report: out/euclid/classification-rate-A.txt out/euclid/classification-rate-B.txt

Report/report.pdf: Report/report.tex report
	latexmk -pdf -cd -use-make -silent -pdflatex='pdflatex -interaction=batchmode -synctex=1' $<

clean:
	rm -rf $(OBJDIR)
	rm -f $(EXEC)
	rm -rf out
	cd Report/; latexmk -c

# Generate .png images from .pgm images. Needed for report, since pdfLaTeX doesn't support .pgm images
%.png: %.pgm
	pnmtopng $< > $@

# Auto-Build .cpp files into .o
$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIRS) $(OBJDIRS)
	$(CXX) $(DEPFLAGS) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

# Make generated directories
$(DEPDIRS) $(OBJDIRS) out out/bayes out/euclid: ; @mkdir -p $@
$(DEPFILES):
include $(wildcard $(DEPFILES))