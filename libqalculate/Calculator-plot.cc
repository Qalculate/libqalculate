/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Calculator.h"
#include "util.h"
#include "MathStructure.h"

#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

using std::string;
using std::vector;
using std::cout;
using std::endl;

PlotParameters::PlotParameters() {
	auto_y_min = true;
	auto_x_min = true;
	auto_y_max = true;
	auto_x_max = true;
	y_log = false;
	x_log = false;
	y_log_base = 10;
	x_log_base = 10;
	grid = false;
	color = true;
	linewidth = -1;
	show_all_borders = false;
	legend_placement = PLOT_LEGEND_TOP_RIGHT;
}
PlotDataParameters::PlotDataParameters() {
	yaxis2 = false;
	xaxis2 = false;
	style = PLOT_STYLE_LINES;
	smoothing = PLOT_SMOOTHING_NONE;
	test_continuous = false;
}

bool Calculator::canPlot() {
#ifdef HAVE_GNUPLOT_CALL
#	ifdef _WIN32
	LPSTR lpFilePart;
	char filename[MAX_PATH];
	return SearchPath(NULL, "gnuplot", ".exe", MAX_PATH, filename, &lpFilePart);
#	else
	FILE *pipe = popen("gnuplot - 2>/dev/null", "w");
	if(!pipe) return false;
	return pclose(pipe) == 0;
#	endif
#else
	return false;
#endif
}

void parse_and_precalculate_plot(string &expression, MathStructure &mstruct, const ParseOptions &po, EvaluationOptions &eo) {
	eo.approximation = APPROXIMATION_APPROXIMATE;
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	eo.parse_options = po2;
	eo.interval_calculation = INTERVAL_CALCULATION_NONE;
	mstruct = CALCULATOR->parse(expression, po2);
	MathStructure mbak(mstruct);
	eo.calculate_functions = false;
	eo.expand = false;
	CALCULATOR->beginTemporaryStopMessages();
	mstruct.eval(eo);
	int im = 0;
	if(CALCULATOR->endTemporaryStopMessages(NULL, &im) > 0 || im > 0) mstruct = mbak;
	eo.calculate_functions = true;
	eo.expand = true;
}

MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, min, max, steps, x_vector, eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	if(y_vector.size() == 0) {
		error(true, _("Unable to generate plot data with current min, max and sampling rate."), NULL);
	}
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, float min, float max, int steps, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	MathStructure min_mstruct(min), max_mstruct(max);
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, steps, x_vector, x_var, po2, msecs));
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, min, max, step, x_vector, eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	if(y_vector.size() == 0) {
		error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
	}
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, float min, float max, float step, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	MathStructure min_mstruct(min), max_mstruct(max), step_mstruct(step);
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, step_mstruct, x_vector, x_var, po2, msecs));
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, x_vector, eo).eval(eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	return y_vector;
}

bool Calculator::plotVectors(PlotParameters *param, const vector<MathStructure> &y_vectors, const vector<MathStructure> &x_vectors, vector<PlotDataParameters*> &pdps, bool persistent, int msecs) {

	string homedir = getLocalTmpDir();
	recursiveMakeDir(homedir);

	string commandline_extra;
	string title;

	if(!param) {
		PlotParameters pp;
		param = &pp;
	}

	string plot;

	if(param->filename.empty()) {
		if(!param->color) {
			commandline_extra += " -mono";
		}
		plot += "set terminal pop\n";
	} else {
		persistent = true;
		if(param->filetype == PLOT_FILETYPE_AUTO) {
			size_t i = param->filename.rfind(".");
			if(i == string::npos) {
				param->filetype = PLOT_FILETYPE_PNG;
				error(false, _("No extension in file name. Saving as PNG image."), NULL);
			} else {
				string ext = param->filename.substr(i + 1, param->filename.length() - (i + 1));
				if(ext == "png") {
					param->filetype = PLOT_FILETYPE_PNG;
				} else if(ext == "ps") {
					param->filetype = PLOT_FILETYPE_PS;
				} else if(ext == "pdf") {
					param->filetype = PLOT_FILETYPE_PDF;
				} else if(ext == "eps") {
					param->filetype = PLOT_FILETYPE_EPS;
				} else if(ext == "svg") {
					param->filetype = PLOT_FILETYPE_SVG;
				} else if(ext == "fig") {
					param->filetype = PLOT_FILETYPE_FIG;
				} else if(ext == "tex") {
					param->filetype = PLOT_FILETYPE_LATEX;
				} else {
					param->filetype = PLOT_FILETYPE_PNG;
					error(false, _("Unknown extension in file name. Saving as PNG image."), NULL);
				}
			}
		}
		plot += "set terminal ";
		switch(param->filetype) {
			case PLOT_FILETYPE_FIG: {
				plot += "fig ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				break;
			}
			case PLOT_FILETYPE_SVG: {
				plot += "svg";
				break;
			}
			case PLOT_FILETYPE_LATEX: {
				plot += "latex ";
				break;
			}
			case PLOT_FILETYPE_PS: {
				plot += "postscript ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				plot += " \"Times\"";
				break;
			}
			case PLOT_FILETYPE_PDF: {
				plot += "pdf ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				break;
			}
			case PLOT_FILETYPE_EPS: {
				plot += "postscript eps ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				plot += " \"Times\"";
				break;
			}
			default: {
				plot += "png ";
				break;
			}

		}
		plot += "\nset output \"";
		plot += param->filename;
		plot += "\"\n";
	}

	plot += "set termoption noenhanced\n";

	switch(param->legend_placement) {
		case PLOT_LEGEND_NONE: {plot += "set nokey\n"; break;}
		case PLOT_LEGEND_TOP_LEFT: {plot += "set key top left\n"; break;}
		case PLOT_LEGEND_TOP_RIGHT: {plot += "set key top right\n"; break;}
		case PLOT_LEGEND_BOTTOM_LEFT: {plot += "set key bottom left\n"; break;}
		case PLOT_LEGEND_BOTTOM_RIGHT: {plot += "set key bottom right\n"; break;}
		case PLOT_LEGEND_BELOW: {plot += "set key below\n"; break;}
		case PLOT_LEGEND_OUTSIDE: {plot += "set key outside\n"; break;}
	}
	if(!param->x_label.empty()) {
		title = param->x_label;
		gsub("\"", "\\\"", title);
		plot += "set xlabel \"";
		plot += title;
		plot += "\"\n";
	}
	if(!param->y_label.empty()) {
		string title = param->y_label;
		gsub("\"", "\\\"", title);
		plot += "set ylabel \"";
		plot += title;
		plot += "\"\n";
	}
	if(!param->title.empty()) {
		title = param->title;
		gsub("\"", "\\\"", title);
		plot += "set title \"";
		plot += title;
		plot += "\"\n";
	}
	if(param->grid) {
		plot += "set grid\n";

	}
	if(!param->auto_y_min || !param->auto_y_max) {
		plot += "set yrange [";
		if(!param->auto_y_min) plot += d2s(param->y_min);
		plot += ":";
		if(!param->auto_y_max) plot += d2s(param->y_max);
		plot += "]";
		plot += "\n";
	}
	if(param->x_log) {
		plot += "set logscale x ";
		plot += i2s(param->x_log_base);
		plot += "\n";
	}
	if(param->y_log) {
		plot += "set logscale y ";
		plot += i2s(param->y_log_base);
		plot += "\n";
	}
	if(param->show_all_borders) {
		plot += "set border 15\n";
	} else {
		bool xaxis2 = false, yaxis2 = false;
		for(size_t i = 0; i < pdps.size(); i++) {
			if(pdps[i] && pdps[i]->xaxis2) {
				xaxis2 = true;
			}
			if(pdps[i] && pdps[i]->yaxis2) {
				yaxis2 = true;
			}
		}
		if(xaxis2 && yaxis2) {
			plot += "set border 15\nset x2tics\nset y2tics\n";
		} else if(xaxis2) {
			plot += "set border 7\nset x2tics\n";
		} else if(yaxis2) {
			plot += "set border 11\nset y2tics\n";
		} else {
			plot += "set border 3\n";
		}
		plot += "set xtics nomirror\nset ytics nomirror\n";
	}
	size_t samples = 1000;
	for(size_t i = 0; i < y_vectors.size(); i++) {
		if(!y_vectors[i].isUndefined()) {
			if(y_vectors[i].size() > 3000) {
				samples = 6000;
				break;
			}
			if(y_vectors[i].size() * 2 > samples) samples = y_vectors[i].size() * 2;
		}
	}
	plot += "set samples ";
	plot += i2s(samples);
	plot += "\n";
	plot += "plot ";
	for(size_t i = 0; i < y_vectors.size(); i++) {
		if(!y_vectors[i].isUndefined()) {
			if(i != 0) {
				plot += ",";
			}
			string filename = "gnuplot_data";
			filename += i2s(i + 1);
			filename = buildPath(homedir, filename);
#ifdef _WIN32
			gsub("\\", "\\\\", filename);
#endif
			plot += "\"";
			plot += filename;
			plot += "\"";
			if(i < pdps.size()) {
				switch(pdps[i]->smoothing) {
					case PLOT_SMOOTHING_UNIQUE: {plot += " smooth unique"; break;}
					case PLOT_SMOOTHING_CSPLINES: {plot += " smooth csplines"; break;}
					case PLOT_SMOOTHING_BEZIER: {plot += " smooth bezier"; break;}
					case PLOT_SMOOTHING_SBEZIER: {plot += " smooth sbezier"; break;}
					default: {}
				}
				if(pdps[i]->xaxis2 && pdps[i]->yaxis2) {
					plot += " axis x2y2";
				} else if(pdps[i]->xaxis2) {
					plot += " axis x2y1";
				} else if(pdps[i]->yaxis2) {
					plot += " axis x1y2";
				}
				if(!pdps[i]->title.empty()) {
					title = pdps[i]->title;
					gsub("\"", "\\\"", title);
					plot += " title \"";
					plot += title;
					plot += "\"";
				}
				switch(pdps[i]->style) {
					case PLOT_STYLE_LINES: {plot += " with lines"; break;}
					case PLOT_STYLE_POINTS: {plot += " with points"; break;}
					case PLOT_STYLE_POINTS_LINES: {plot += " with linespoints"; break;}
					case PLOT_STYLE_BOXES: {plot += " with boxes"; break;}
					case PLOT_STYLE_HISTOGRAM: {plot += " with histeps"; break;}
					case PLOT_STYLE_STEPS: {plot += " with steps"; break;}
					case PLOT_STYLE_CANDLESTICKS: {plot += " with candlesticks"; break;}
					case PLOT_STYLE_DOTS: {plot += " with dots"; break;}
				}
				if(param->linewidth < 1) {
					plot += " lw 2";
				} else {
					plot += " lw ";
					plot += i2s(param->linewidth);
				}
			}
		}
	}
	plot += "\n";

	string plot_data;
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_MIDPOINT;
	po.decimalpoint_sign = ".";
	po.comma_sign = ",";
	for(size_t serie = 0; serie < y_vectors.size(); serie++) {
		if(!y_vectors[serie].isUndefined()) {
			string filename = "gnuplot_data";
			filename += i2s(serie + 1);
			string filepath = buildPath(homedir, filename);
			FILE *fdata = fopen(filepath.c_str(), "w+");
			if(!fdata) {
				error(true, _("Could not create temporary file %s"), filepath.c_str(), NULL);
				return false;
			}
			plot_data = "";
			int non_numerical = 0, non_real = 0;
			//string str = "";
			if(msecs > 0) startControl(msecs);
			ComparisonResult ct1 = COMPARISON_RESULT_EQUAL, ct2 = COMPARISON_RESULT_EQUAL;
			size_t last_index = string::npos, last_index2 = string::npos;
			bool check_continuous = pdps[serie]->test_continuous && (pdps[serie]->style == PLOT_STYLE_LINES || pdps[serie]->style == PLOT_STYLE_POINTS_LINES);
			bool prev_failed = false;
			for(size_t i = 1; i <= y_vectors[serie].countChildren(); i++) {
				ComparisonResult ct = COMPARISON_RESULT_UNKNOWN;
				bool invalid_nr = false, b_imagzero_x = false, b_imagzero_y = false;
				if(!y_vectors[serie].getChild(i)->isNumber()) {
					invalid_nr = true;
					non_numerical++;
					//if(non_numerical == 1) str = y_vectors[serie].getChild(i)->print(po);
				} else if(!y_vectors[serie].getChild(i)->number().isReal()) {
					b_imagzero_y = testComplexZero(&y_vectors[serie].getChild(i)->number(), y_vectors[serie].getChild(i)->number().internalImaginary());
					if(!b_imagzero_y) {
						invalid_nr = true;
						non_real++;
						//if(non_numerical + non_real == 1) str = y_vectors[serie].getChild(i)->print(po);
					}
				}
				if(serie < x_vectors.size() && !x_vectors[serie].isUndefined() && x_vectors[serie].countChildren() == y_vectors[serie].countChildren()) {
					if(!x_vectors[serie].getChild(i)->isNumber()) {
						invalid_nr = true;
						non_numerical++;
						//if(non_numerical == 1) str = x_vectors[serie].getChild(i)->print(po);
					} else if(!x_vectors[serie].getChild(i)->number().isReal()) {
						b_imagzero_x = testComplexZero(&x_vectors[serie].getChild(i)->number(), x_vectors[serie].getChild(i)->number().internalImaginary());
						if(!b_imagzero_x) {
							invalid_nr = true;
							non_real++;
							//if(non_numerical + non_real == 1) str = x_vectors[serie].getChild(i)->print(po);
						}
					}
					if(!invalid_nr) {
						if(b_imagzero_y) plot_data += x_vectors[serie].getChild(i)->number().realPart().print(po);
						else plot_data += x_vectors[serie].getChild(i)->print(po);
						plot_data += " ";
					}
				}
				if(!invalid_nr) {
					if(check_continuous && !prev_failed) {
						if(i == 1 || ct2 == COMPARISON_RESULT_UNKNOWN) ct = COMPARISON_RESULT_EQUAL;
						else ct = y_vectors[serie].getChild(i - 1)->number().compare(y_vectors[serie].getChild(i)->number());
						if((ct == COMPARISON_RESULT_GREATER || ct == COMPARISON_RESULT_LESS) && (ct1 == COMPARISON_RESULT_GREATER || ct1 == COMPARISON_RESULT_LESS) && (ct2 == COMPARISON_RESULT_GREATER || ct2 == COMPARISON_RESULT_LESS) && ct1 != ct2 && ct != ct2) {
							if(last_index2 != string::npos) plot_data.insert(last_index2 + 1, "  \n");
						}
					}
					if(b_imagzero_x) plot_data += y_vectors[serie].getChild(i)->number().realPart().print(po);
					else plot_data += y_vectors[serie].getChild(i)->print(po);
					plot_data += "\n";
					prev_failed = false;
				} else if(!prev_failed) {
					ct = COMPARISON_RESULT_UNKNOWN;
					plot_data += "  \n";
					prev_failed = true;
				}
				last_index2 = last_index;
				last_index = plot_data.length() - 1;
				ct1 = ct2;
				ct2 = ct;
				if(aborted()) {
					fclose(fdata);
					if(msecs > 0) {
						error(true, _("It took too long to generate the plot data."), NULL);
						stopControl();
					}
					return false;
				}
			}
			if(msecs > 0) stopControl();
			/*if(non_numerical > 0 || non_real > 0) {
				string stitle;
				if(serie < pdps.size() && !pdps[serie]->title.empty()) {
					stitle = pdps[serie]->title.c_str();
				} else {
					stitle = i2s(serie).c_str();
				}
				if(non_numerical > 0) {
					error(true, _("Series %s contains non-numerical data (\"%s\" first of %s) which can not be properly plotted."), stitle.c_str(), str.c_str(), i2s(non_numerical).c_str(), NULL);
				} else {
					error(true, _("Series %s contains non-real data (\"%s\" first of %s) which can not be properly plotted."), stitle.c_str(), str.c_str(), i2s(non_real).c_str(), NULL);
				}
			}*/
			fputs(plot_data.c_str(), fdata);
			fflush(fdata);
			fclose(fdata);
		}
	}

	return invokeGnuplot(plot, commandline_extra, persistent);
}
#ifdef HAVE_GNUPLOT_CALL
bool Calculator::invokeGnuplot(string commands, string commandline_extra, bool persistent) {
	FILE *pipe = NULL;
	if(!b_gnuplot_open || !gnuplot_pipe || persistent || commandline_extra != gnuplot_cmdline) {
		if(!persistent) {
			closeGnuplot();
		}
		string commandline = "gnuplot";
		if(persistent) {
			commandline += " -persist";
		}
		commandline += commandline_extra;
#ifdef _WIN32
		commandline += " - 2>nul";
		pipe = _popen(commandline.c_str(), "w");
#else
		commandline += " - 2>/dev/null";
		pipe = popen(commandline.c_str(), "w");
#endif
		if(!pipe) {
			error(true, _("Failed to invoke gnuplot. Make sure that you have gnuplot installed in your path."), NULL);
			return false;
		}
		if(!persistent && pipe) {
			gnuplot_pipe = pipe;
			b_gnuplot_open = true;
			gnuplot_cmdline = commandline_extra;
		}
	} else {
		pipe = gnuplot_pipe;
	}
	if(!pipe) {
		return false;
	}
	if(!persistent) {
		fputs("clear\n", pipe);
		fputs("reset\n", pipe);
	}
	fputs(commands.c_str(), pipe);
	fflush(pipe);
	if(persistent) {
		return pclose(pipe) == 0;
	}
	return true;
}
#else
bool Calculator::invokeGnuplot(string, string, bool) {
	return false;
}
#endif
bool Calculator::closeGnuplot() {
#ifdef HAVE_GNUPLOT_CALL
	if(gnuplot_pipe) {
#	ifdef _WIN32
		int rv = _pclose(gnuplot_pipe);
#	else
		int rv = pclose(gnuplot_pipe);
#	endif
		gnuplot_pipe = NULL;
		b_gnuplot_open = false;
		return rv == 0;
	}
	gnuplot_pipe = NULL;
	b_gnuplot_open = false;
	return true;
#else
	return false;
#endif
}
bool Calculator::gnuplotOpen() {
	return b_gnuplot_open && gnuplot_pipe;
}

