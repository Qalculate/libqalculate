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
#include "Calculator_p.h"
#include "util.h"
#include "MathStructure.h"
#include "MathStructure-support.h"

#include <locale.h>
#ifdef _MSC_VER
#	include <sys/utime.h>
#else
#	include <unistd.h>
#	include <utime.h>
#endif
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
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
#	ifdef HAVE_BYO_GNUPLOT
	return true;
#	else
	return false;
#	endif
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
	return expressionToPlotVector(expression, min, max, steps, true, x_vector, x_var, po, msecs);
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, int steps, bool separate_complex_part, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	eo.allow_complex = separate_complex_part;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure x_v;
	MathStructure y_vector;
	generate_plotvector(mparse, x_mstruct, min, max, steps, x_vector ? *x_vector : x_v, y_vector, eo);
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
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, steps, true, x_vector, x_var, po2, msecs));
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	return expressionToPlotVector(expression, min, max, step, true, x_vector, x_var, po, msecs);
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, bool separate_complex_part, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	eo.allow_complex = separate_complex_part;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure x_v;
	MathStructure y_vector;
	generate_plotvector(mparse, x_mstruct, min, max, step, x_vector ? *x_vector : x_v, y_vector, eo);
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
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, step_mstruct, true, x_vector, x_var, po2, msecs));
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

#ifdef HAVE_GNUPLOT_CALL
bool Calculator::invokeGnuplot(string commands, string commandline_extra, bool persistent) {
	if(priv->persistent_plot) persistent = true;
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
#	ifdef HAVE_BYO_GNUPLOT
bool qalc_invoke_gnuplot(vector<std::pair<string, string>>, string, string, bool);
string qalc_gnuplot_data_dir();
#	else
bool Calculator::invokeGnuplot(string, string, bool) {
	return false;
}
#	endif
#endif

void Calculator::forcePersistentPlot(bool persistent) {
	priv->persistent_plot = persistent;
}

bool Calculator::plotVectors(PlotParameters *param, const vector<MathStructure> &y_vectors, const vector<MathStructure> &x_vectors, vector<PlotDataParameters*> &pdps, bool persistent, int msecs) {

	string homedir;
#ifdef HAVE_GNUPLOT_CALL
	homedir = getLocalTmpDir();
	recursiveMakeDir(homedir);
#else
#	ifdef HAVE_BYO_GNUPLOT
	homedir = qalc_gnuplot_data_dir();
#	else
	return false;
#	endif
#endif

	string commandline_extra;
	string title;

	PlotParameters pp;
	if(!param) param = &pp;

	vector<MathStructure> yim_vectors;
	for(size_t i = 0; i < y_vectors.size(); i++) {
		yim_vectors.push_back(m_undefined);
		if(!y_vectors[i].isUndefined()) {
			for(size_t i2 = 0; i2 < y_vectors[i].size() - 1; i2++) {
				if(y_vectors[i][i2].isNumber() && y_vectors[i][i2].number().hasImaginaryPart()) {
					if(y_vectors[i][i2 + 1].isNumber() && y_vectors[i][i2 + 1].number().hasImaginaryPart()) {
						yim_vectors[i].clearVector();
						yim_vectors[i].resizeVector(y_vectors[i].size(), m_zero);
						for(i2 = 0; i2 < y_vectors[i].size(); i2++) {
							if(y_vectors[i][i2].isNumber()) {
								if(y_vectors[i][i2].number().hasImaginaryPart()) {
									yim_vectors[i][i2].number() = y_vectors[i][i2].number().imaginaryPart();
								}
							} else {
								yim_vectors[i][i2].setUndefined();
							}
						}
						break;
					}
				}
			}
		}
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

	plot += "set encoding utf8\n";

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

	vector<MathStructure> munit;
	string sunit;
	if(!param->y_label.empty()) sunit = "0";
	for(size_t serie = 0; serie < y_vectors.size(); serie++) {
		if(y_vectors[serie].isUndefined()) {
			munit.push_back(m_undefined);
		} else {
			munit.push_back(m_zero);
			if(y_vectors[serie].size() > 0 && !y_vectors[serie][0].isNumber()) {
				if(is_unit_multiexp(y_vectors[serie][0])) {
					munit[serie] = y_vectors[serie][0];
				} else if(y_vectors[serie][0].isMultiplication() && y_vectors[serie][0].size() >= 2 && y_vectors[serie][0][0].isNumber()) {
					munit[serie] = y_vectors[serie][0];
					munit[serie].delChild(1, true);
					if(!is_unit_multiexp(munit[serie])) munit[serie].clear();
				}
			}
			if(sunit != "0") {
				if(munit[serie].isZero()) {
					sunit = "0";
				} else {
					string str = munit[serie].print();
					if(sunit.empty()) sunit = str;
					else if(str != sunit) sunit = "0";
				}
			}
		}
	}
	if(!param->y_label.empty() || (!sunit.empty() && sunit != "0")) {
		string title = param->y_label;
		if(title.empty()) title = sunit;
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
	bool b_polar = false;
	for(size_t i = 0; i < pdps.size(); i++) {
		if(pdps[i]->style == PLOT_STYLE_POLAR) {
			b_polar = true;
			plot += "set polar\n";
			break;
		}
	}
	if(param->grid) {
		if(b_polar) plot += "set grid polar";
		else plot += "set grid";
		if(param->grid > 1) {
			plot += " lw ";
			plot += i2s(param->grid);
		}
		plot += "\n";
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
	size_t file_index = 1;
	for(size_t i_pre = 0; i_pre < y_vectors.size() * 2; i_pre++) {
		size_t i = i_pre / 2;
		if((i_pre % 2 == 0 && !y_vectors[i].isUndefined()) || (i_pre % 2 == 1 && !yim_vectors[i].isUndefined())) {
			if(file_index != 1) {
				plot += ",";
			}
			string filename = "gnuplot_data";
			filename += i2s(file_index);
			file_index++;
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
					if(i_pre % 2 == 1) {title += " : "; title += _("imaginary part");}
					else if(!yim_vectors[i].isUndefined()) {title += " : "; title += _("real part");}
					plot += " title \"";
					plot += title;
					plot += "\"";
				}
				switch(pdps[i]->style) {
					case PLOT_STYLE_POINTS: {plot += " with points"; break;}
					case PLOT_STYLE_POINTS_LINES: {plot += " with linespoints"; break;}
					case PLOT_STYLE_BOXES: {plot += " with boxes"; break;}
					case PLOT_STYLE_HISTOGRAM: {plot += " with histeps"; break;}
					case PLOT_STYLE_STEPS: {plot += " with steps"; break;}
					case PLOT_STYLE_CANDLESTICKS: {plot += " with candlesticks"; break;}
					case PLOT_STYLE_DOTS: {plot += " with dots"; break;}
					default: {plot += " with lines"; break;}
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
#ifndef HAVE_GNUPLOT_CALL
	vector<std::pair<string, string>> data_files;
#endif
	MathStructure m, mprev;
	file_index = 1;
	for(size_t i_pre = 0; i_pre < y_vectors.size() * 2; i_pre++) {
		size_t serie = i_pre / 2;
		if((i_pre % 2 == 0 && !y_vectors[serie].isUndefined()) || (i_pre % 2 == 1 && !yim_vectors[serie].isUndefined())) {
			string filename = "gnuplot_data";
			filename += i2s(file_index);
			file_index++;
#ifdef HAVE_GNUPLOT_CALL
			string filepath = buildPath(homedir, filename);
			FILE *fdata = fopen(filepath.c_str(), "w+");
			if(!fdata) {
				error(true, _("Could not create temporary file %s"), filepath.c_str(), NULL);
				return false;
			}
#else
			filename = buildPath(homedir, filename);
#endif
			plot_data = "";
			if(msecs > 0) startControl(msecs);
			ComparisonResult ct1 = COMPARISON_RESULT_EQUAL, ct2 = COMPARISON_RESULT_EQUAL;
			size_t last_index = string::npos, last_index2 = string::npos;
			bool check_continuous = pdps[serie]->test_continuous && (pdps[serie]->style == PLOT_STYLE_LINES || pdps[serie]->style == PLOT_STYLE_POINTS_LINES);
			size_t discontinuous_count = 0, point_count = 0;
			bool discontinuous_start = false, discontinuous_end = false;
			bool prev_failed = false;
			const MathStructure *yprev = NULL;
			for(size_t i = 1; (i_pre % 2 == 0 && i <= y_vectors[serie].countChildren()) || (i_pre % 2 == 1 && i <= yim_vectors[serie].countChildren()); i++) {
				bool b_real = (i_pre % 2 == 0 && !yim_vectors[serie].isUndefined());
				ComparisonResult ct = COMPARISON_RESULT_UNKNOWN;
				bool invalid_nr = false, b_imagzero_x = false, b_imagzero_y = false;
				const MathStructure *yvalue;
				if(i_pre % 2 == 1) yvalue = yim_vectors[serie].getChild(i);
				else yvalue = y_vectors[serie].getChild(i);
				if(!yvalue->isNumber() && !munit[serie].isZero()) {
					if(yvalue->isMultiplication() && yvalue->size() >= 2 && yvalue->getChild(1)->isNumber()) {
						bool b = false;
						if(yvalue->size() == 2 && yvalue->getChild(2)->equals(munit[serie])) {
							b = true;
						} else if(yvalue->size() > 2 && munit[serie].isMultiplication() && munit[serie].size() == yvalue->size() - 1) {
							b = true;
							for(size_t i2 = 0; i2 < munit[serie].size(); i2++) {
								if(!yvalue->getChild(i2 + 2)->equals(munit[serie][i2])) {
									b = false;
									break;
								}
							}
						}
						if(b) {
							m = *yvalue->getChild(1);
							yvalue = &m;
						}
					} else if(yvalue->equals(munit[serie])) {
						m.set(1, 1, 0);
						yvalue = &m;
					}
				}
				if(!yvalue->isNumber()) {
					invalid_nr = true;
				} else if(i_pre % 2 == 0 && !yvalue->number().isReal()) {
					b_imagzero_y = b_real || testComplexZero(&yvalue->number(), yvalue->number().internalImaginary());
					if(!b_imagzero_y) {
						invalid_nr = true;
					}
				}
				if(serie < x_vectors.size() && !x_vectors[serie].isUndefined() && x_vectors[serie].countChildren() == y_vectors[serie].countChildren()) {
					if(!x_vectors[serie].getChild(i)->isNumber()) {
						invalid_nr = true;
					} else if(!x_vectors[serie].getChild(i)->number().isReal()) {
						b_imagzero_x = testComplexZero(&x_vectors[serie].getChild(i)->number(), x_vectors[serie].getChild(i)->number().internalImaginary());
						if(!b_imagzero_x) {
							invalid_nr = true;
						}
					}
					if(!invalid_nr) {
						if(b_imagzero_x) plot_data += x_vectors[serie].getChild(i)->number().realPart().print(po);
						else plot_data += x_vectors[serie].getChild(i)->print(po);
						plot_data += " ";
						point_count++;
					}
				}
				if(!invalid_nr) {
					if(check_continuous && !prev_failed) {
						if(i == 1 || ct2 == COMPARISON_RESULT_UNKNOWN) ct = COMPARISON_RESULT_EQUAL;
						else ct = yprev->number().compare(yvalue->number());
						if((ct == COMPARISON_RESULT_GREATER || ct == COMPARISON_RESULT_LESS) && (ct1 == COMPARISON_RESULT_GREATER || ct1 == COMPARISON_RESULT_LESS) && (ct2 == COMPARISON_RESULT_GREATER || ct2 == COMPARISON_RESULT_LESS) && ct1 != ct2 && ct != ct2) {
							if(last_index2 != string::npos) {
								plot_data.insert(last_index2 + 1, "  \n");
								discontinuous_count++;
								last_index += 3;
								if(i == 4) {
									discontinuous_start = true;
								} else if(i == 5 && discontinuous_start && plot_data.find("\n") != plot_data.find("\n  \n")) {
									size_t i_firstrow = plot_data.find("\n");
									plot_data.insert(i_firstrow, "\n  ");
									last_index += 3;
								}
								if((i_pre % 2 == 0 && i == y_vectors[serie].countChildren() - 1) || (i_pre % 2 == 1 && i == yim_vectors[serie].countChildren() - 1)) {
									discontinuous_end = true;
								} else if(discontinuous_end && ((i_pre % 2 == 0 && i == y_vectors[serie].countChildren()) || (i_pre % 2 == 1 && i == yim_vectors[serie].countChildren()))) {
									size_t i_lastrow = plot_data.rfind("\n");
									plot_data.insert(i_lastrow, "\n  ");
								}
							}
						}
					}
					if(b_imagzero_y) plot_data += yvalue->number().realPart().print(po);
					else plot_data += yvalue->print(po);
					plot_data += "\n";
					prev_failed = false;
				} else if(!prev_failed) {
					ct = COMPARISON_RESULT_UNKNOWN;
					plot_data += "  \n";
					discontinuous_count++;
					prev_failed = true;
				}
				if(yprev == &m) {
					mprev = m;
					yprev = &mprev;
					m.clear();
				} else {
					yprev = yvalue;
				}
				last_index2 = last_index;
				last_index = plot_data.length() - 1;
				ct1 = ct2;
				ct2 = ct;
				if(aborted()) {
#ifdef HAVE_GNUPLOT_CALL
					fclose(fdata);
#endif
					if(msecs > 0) {
						error(true, _("It took too long to generate the plot data."), NULL);
						stopControl();
					}
					return false;
				}
			}
			if(msecs > 0) stopControl();
			if(check_continuous && pdps[serie]->style == PLOT_STYLE_LINES && discontinuous_count > point_count / 2) {
				if((i_pre % 2 == 0 && y_vectors[serie].countChildren() > 100) || (i_pre % 2 == 1 && yim_vectors[serie].countChildren() > 100)) gsub(" with lines", " with dots", plot);
				else gsub(" with lines", " with points", plot);
			}
#ifdef HAVE_GNUPLOT_CALL
			fputs(plot_data.c_str(), fdata);
			fflush(fdata);
			fclose(fdata);
#else
			data_files.push_back(std::make_pair(filename, plot_data));
#endif
		}
	}
#ifdef HAVE_GNUPLOT_CALL
	return invokeGnuplot(plot, commandline_extra, persistent);
#else
#	ifdef HAVE_BYO_GNUPLOT
	return qalc_invoke_gnuplot(data_files, plot, commandline_extra, persistent);
#	endif
	return false;
#endif
}
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
#	ifdef HAVE_BYO_GNUPLOT
	return true;
#	else
	return false;
#	endif
#endif
}
bool Calculator::gnuplotOpen() {
	return b_gnuplot_open && gnuplot_pipe;
}
