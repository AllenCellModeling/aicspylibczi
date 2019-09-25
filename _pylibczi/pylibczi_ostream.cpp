//
// Created by Jamie Sherman on 2019-09-19.
//
#include "pylibczi_ostream.h"

ostream& operator<<(ostream& out, const libCZI::CDimCoordinate& cdim)
{
	stringstream tmp;
	cdim.EnumValidDimensions([&tmp](libCZI::DimensionIndex di, int val) {
		tmp << (tmp.str().empty() ? "CDimCoordinate: {" : ", ");
		tmp << libCZI::Utils::DimensionToChar(di) << ": " << val;
		return true;
	});
	tmp << "}";
	out << tmp.str();
	return out;
}

ostream& operator<<(ostream& out, const libCZI::CDimBounds& bounds)
{
	stringstream tmp;
	bounds.EnumValidDimensions([&tmp](libCZI::DimensionIndex di, int st, int len) {
		tmp << (tmp.str().empty() ? "CDimBounds: {" : ", ");
		tmp << libCZI::Utils::DimensionToChar(di) << ": (" << st << "," << len << ")";
		return true;
	});
	tmp << "}";
	out << tmp.str();

	return out;
}
