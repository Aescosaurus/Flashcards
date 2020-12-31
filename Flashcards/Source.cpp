#include "Flashcard.h"
#include <iostream>

int main()
{
	// std::wifstream in{ "test.txt" };
	// std::wstring text;
	// while( in.good() )
	// {
	// 	text += in.get();
	// }
	// 
	// std::wofstream out{ "blah.txt" };
	// out << text;

	Flashcard fc;

	fc.UpdateScores();
	fc.Grade();
	fc.GenerateReview( 1 );

	return( 0 );
}