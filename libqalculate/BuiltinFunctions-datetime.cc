/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016, 2018  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "BuiltinFunctions.h"
#include "util.h"
#include "MathStructure.h"
#include "Number.h"
#include "Calculator.h"
#include "Unit.h"
#include "QalculateDateTime.h"

#include <sstream>
#include <time.h>
#include <limits>
#include <algorithm>

using std::string;
using std::cout;
using std::vector;
using std::endl;

int calender_to_id(const string &str) {
	if(str == "1" || equalsIgnoreCase(str, "gregorian") || equalsIgnoreCase(str, _("gregorian"))) return CALENDAR_GREGORIAN;
	if(str == "8" || equalsIgnoreCase(str, "milankovic") || equalsIgnoreCase(str, "milanković") || equalsIgnoreCase(str, _("milankovic"))) return CALENDAR_MILANKOVIC;
	if(str == "7" || equalsIgnoreCase(str, "julian") || equalsIgnoreCase(str, _("julian"))) return CALENDAR_JULIAN;
	if(str == "3" || equalsIgnoreCase(str, "islamic") || equalsIgnoreCase(str, _("islamic"))) return CALENDAR_ISLAMIC;
	if(str == "2" || equalsIgnoreCase(str, "hebrew") || equalsIgnoreCase(str, _("hebrew"))) return CALENDAR_HEBREW;
	if(str == "11" || equalsIgnoreCase(str, "egyptian") || equalsIgnoreCase(str, _("egyptian"))) return CALENDAR_EGYPTIAN;
	if(str == "4" || equalsIgnoreCase(str, "persian") || equalsIgnoreCase(str, _("persian"))) return CALENDAR_PERSIAN;
	if(str == "9" || equalsIgnoreCase(str, "coptic") || equalsIgnoreCase(str, _("coptic"))) return CALENDAR_COPTIC;
	if(str == "10" || equalsIgnoreCase(str, "ethiopian") || equalsIgnoreCase(str, _("ethiopian"))) return CALENDAR_ETHIOPIAN;
	if(str == "5" || equalsIgnoreCase(str, "indian") || equalsIgnoreCase(str, _("indian"))) return CALENDAR_INDIAN;
	if(str == "6" || equalsIgnoreCase(str, "chinese") || equalsIgnoreCase(str, _("chinese"))) return CALENDAR_CHINESE;
	return -1;
}

DateFunction::DateFunction() : MathFunction("date", 1, 4) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	IntegerArgument *iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	Number fr(1, 1, 0);
	iarg->setMin(&fr);
	fr.set(24, 1, 0);
	iarg->setMax(&fr);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "1");
	iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	fr.set(1, 1, 0);
	iarg->setMin(&fr);
	fr.set(31, 1, 0);
	iarg->setMax(&fr);
	setDefaultValue(3, "1");
	setArgumentDefinition(3, iarg);
	TextArgument *targ = new TextArgument();
	setArgumentDefinition(4, targ);
	setDefaultValue(4, _("gregorian"));
}
int DateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	int ct = calender_to_id(vargs[3].symbol());
	if(ct < 0) {
		CALCULATOR->error(true, "Unrecognized calendar.", NULL);
		return 0;
	}
	QalculateDateTime dt;
	if(!calendarToDate(dt, vargs[0].number().lintValue(), vargs[1].number().lintValue(), vargs[2].number().lintValue(), (CalendarSystem) ct)) return 0;
	mstruct.set(dt);
	return 1;
}
DateTimeFunction::DateTimeFunction() : MathFunction("datetime", 1, 6) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	IntegerArgument *iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	Number fr(1, 1, 0);
	iarg->setMin(&fr);
	fr.set(12, 1, 0);
	iarg->setMax(&fr);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "1");
	iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	fr.set(1, 1, 0);
	iarg->setMin(&fr);
	fr.set(31, 1, 0);
	iarg->setMax(&fr);
	setDefaultValue(3, "1");
	setArgumentDefinition(3, iarg);
	iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	iarg->setMin(&nr_zero);
	fr.set(23, 1, 0);
	iarg->setMax(&fr);
	setArgumentDefinition(4, iarg);
	setDefaultValue(4, "0");
	iarg = new IntegerArgument();
	iarg->setHandleVector(false);
	iarg->setMin(&nr_zero);
	fr.set(59, 1, 0);
	iarg->setMax(&fr);
	setArgumentDefinition(5, iarg);
	setDefaultValue(5, "0");
	NumberArgument *narg = new NumberArgument();
	narg->setHandleVector(false);
	narg->setMin(&nr_zero);
	fr.set(61, 1, 0);
	narg->setMax(&fr);
	narg->setIncludeEqualsMax(false);
	setArgumentDefinition(6, narg);
	setDefaultValue(6, "0");
}
int DateTimeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime dt;
	if(!dt.set(vargs[0].number().lintValue(), vargs[1].number().lintValue(), vargs[2].number().lintValue())) return 0;
	if(!vargs[3].isZero() || !vargs[4].isZero() || !vargs[5].isZero()) {
		if(!dt.setTime(vargs[3].number().lintValue(), vargs[4].number().lintValue(), vargs[5].number())) return 0;
	}
	mstruct.set(dt);
	return 1;
}
TimeValueFunction::TimeValueFunction() : MathFunction("timevalue", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "now");
}
int TimeValueFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	Number nr(vargs[0].datetime()->second());
	nr /= 60;
	nr += vargs[0].datetime()->minute();
	nr /= 60;
	nr += vargs[0].datetime()->hour();
	mstruct.set(nr);
	return 1;
}
TimestampFunction::TimestampFunction() : MathFunction("timestamp", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "now");
}
int TimestampFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	Number nr(date.timestamp());
	if(nr.isInfinite()) return 0;
	mstruct.set(nr);
	return 1;
}
TimestampToDateFunction::TimestampToDateFunction() : MathFunction("stamptodate", 1) {
	//setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
}
int TimestampToDateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if((mstruct.isUnit() && mstruct.unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_SECOND)) || (mstruct.isMultiplication() && mstruct.size() >= 2 && mstruct.last().isUnit() && mstruct.last().unit()->baseUnit() == CALCULATOR->getUnitById(UNIT_ID_SECOND))) {
		Unit *u = NULL;
		if(mstruct.isUnit()) {
			u = mstruct.unit();
			mstruct.set(1, 1, 0, true);
		} else {
			u = mstruct.last().unit();
			mstruct.delChild(mstruct.size(), true);
		}
		if(u != CALCULATOR->getUnitById(UNIT_ID_SECOND)) {
			u->convertToBaseUnit(mstruct);
			mstruct.eval(eo);
		}
	}
	if(!mstruct.isNumber() || !mstruct.number().isReal() || mstruct.number().isInterval()) return -1;
	QalculateDateTime date;
	if(!date.set(mstruct.number())) return -1;
	mstruct.set(date, true);
	return 1;
}

AddDaysFunction::AddDaysFunction() : MathFunction("addDays", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new NumberArgument());
}
int AddDaysFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	if(!mstruct.datetime()->addDays(vargs[1].number())) return 0;
	return 1;
}
AddMonthsFunction::AddMonthsFunction() : MathFunction("addMonths", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new NumberArgument());
}
int AddMonthsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	if(!mstruct.datetime()->addMonths(vargs[1].number())) return 0;
	return 1;
}
AddYearsFunction::AddYearsFunction() : MathFunction("addYears", 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new NumberArgument());
}
int AddYearsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	if(!mstruct.datetime()->addYears(vargs[1].number())) return 0;
	return 1;
}

DaysFunction::DaysFunction() : MathFunction("days", 2, 4) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new DateArgument());
	IntegerArgument *arg = new IntegerArgument();
	Number integ;
	arg->setMin(&integ);
	integ.set(4, 1, 0);
	arg->setMax(&integ);
	setArgumentDefinition(3, arg);
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(3, "1");
}
int DaysFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date1(*vargs[0].datetime()), date2(*vargs[1].datetime());
	Number days(date1.daysTo(date2, vargs[2].number().intValue(), vargs[3].number().isZero()));
	if(days.isInfinite()) return 0;
	days.abs();
	mstruct.set(days);
	return 1;
}
YearFracFunction::YearFracFunction() : MathFunction("yearfrac", 2, 4) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new DateArgument());
	IntegerArgument *arg = new IntegerArgument();
	Number integ;
	arg->setMin(&integ);
	integ.set(4, 1, 0);
	arg->setMax(&integ);
	setArgumentDefinition(3, arg);
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(3, "1");
}
int YearFracFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date1(*vargs[0].datetime()), date2(*vargs[1].datetime());
	Number years(date1.yearsTo(date2, vargs[2].number().intValue(), vargs[3].number().isZero()));
	if(years.isInfinite()) return 0;
	years.abs();
	mstruct.set(years);
	return 1;
}
WeekFunction::WeekFunction() : MathFunction("week", 0, 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(1, "today");
}
int WeekFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	int w = date.week(vargs[1].number().getBoolean());
	if(w < 0) return 0;
	mstruct.set(w, 1, 0);
	return 1;
}
WeekdayFunction::WeekdayFunction() : MathFunction("weekday", 0, 2) {
	setArgumentDefinition(1, new DateArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(1, "today");
}
int WeekdayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	int w = date.weekday();
	if(w < 0) return 0;
	if(vargs[1].number().getBoolean()) {
		if(w == 7) w = 1;
		else w++;
	}
	mstruct.set(w, 1, 0);
	return 1;
}
YeardayFunction::YeardayFunction() : MathFunction("yearday", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int YeardayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	int yd = date.yearday();
	if(yd < 0) return 0;
	mstruct.set(yd, 1, 0);
	return 1;
}
MonthFunction::MonthFunction() : MathFunction("month", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int MonthFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	mstruct.set(date.month(), 1L, 0L);
	return 1;
}
DayFunction::DayFunction() : MathFunction("day", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int DayFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	mstruct.set(date.day(), 1L, 0L);
	return 1;
}
YearFunction::YearFunction() : MathFunction("year", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "today");
}
int YearFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	QalculateDateTime date(*vargs[0].datetime());
	mstruct.set(date.year(), 1L, 0L);
	return 1;
}
TimeFunction::TimeFunction() : MathFunction("time", 0) {
}
int TimeFunction::calculate(MathStructure &mstruct, const MathStructure&, const EvaluationOptions&) {
	int hour, min, sec;
	now(hour, min, sec);
	Number tnr(sec, 1, 0);
	tnr /= 60;
	tnr += min;
	tnr /= 60;
	tnr += hour;
	mstruct = tnr;
	return 1;
}
LunarPhaseFunction::LunarPhaseFunction() : MathFunction("lunarphase", 0, 1) {
	setArgumentDefinition(1, new DateArgument());
	setDefaultValue(1, "now");
}
int LunarPhaseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = lunarPhase(*vargs[0].datetime());
	if(CALCULATOR->aborted()) return 0;
	return 1;
}
NextLunarPhaseFunction::NextLunarPhaseFunction() : MathFunction("nextlunarphase", 1, 2) {
	NumberArgument *arg = new NumberArgument();
	Number fr;
	arg->setMin(&fr);
	fr.set(1, 1, 0);
	arg->setMax(&fr);
	arg->setIncludeEqualsMin(true);
	arg->setIncludeEqualsMax(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setArgumentDefinition(2, new DateArgument());
	setDefaultValue(2, "now");
}
int NextLunarPhaseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = findNextLunarPhase(*vargs[1].datetime(), vargs[0].number());
	if(CALCULATOR->aborted()) return 0;
	return 1;
}

