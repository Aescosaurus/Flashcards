#pragma once

#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <random>

class Flashcard
{
private:
	class Card
	{
	public:
		Card( const std::wstring& line )
		{
			std::vector<std::wstring> words;
			words.emplace_back( L"" );
			for( wchar_t c : line )
			{
				if( c == '=' ) words.emplace_back( L"" );
				else words.back() += c;
			}

			assert( words.size() >= 2 );
			english = words[0];
			japanese = words[1];
			if( isalpha( japanese[0] ) || ( !isalpha( english[0] ) && !isalpha( english[1] ) ) )
			{
				swapRequired = true;
				std::swap( english,japanese );
			}
			try
			{
				if( words.size() > 2 ) score = std::stof( words[2] );
			}
			catch( ... )
			{
				// don't care
			}
		}

		void MoveHint()
		{
			std::wstring hint = L"";
			std::wstring tempJ = L"";
			bool started = false;
			for( wchar_t wc : japanese )
			{
				if( wc == '(' ) started = true;
				// if( wc == ')' ) break;
				if( started ) hint += wc;
				else tempJ += wc;
			}
			japanese = tempJ;
			english = english + L' ' + hint;
		}
		std::wstring GetHint() const
		{
			std::wstring hint = L"";
			bool started = false;
			for( wchar_t wc : japanese )
			{
				if( wc == '(' ) started = true;
				// if( wc == ')' ) break;
				if( started ) hint += wc;
			}
			return( hint );
		}
	public:
		std::wstring english;
		std::wstring japanese;
		float score = 0.0f;
		bool swapRequired = false;
	};
public:
	Flashcard()
	{
		cards = ReadFile( wordBankPath );
	}

	void UpdateScores()
	{
		for( auto& card : cards )
		{
			card.score -= refreshPenalty;
			if( card.score < 0.0f ) card.score = 0.0f;
		}
	}

	// 0 = old (random)
	// 1 = review lowest
	void GenerateReview( int reviewMode )
	{
		std::vector<Card> engList;
		std::vector<Card> jpnList;
		int cardCount = 0;
		auto rndCards = cards;
		std::mt19937 rng{ std::random_device{}( ) };

		if( reviewMode == 0 )
		{
			std::shuffle( rndCards.begin(),rndCards.end(),rng );
		}
		else if( reviewMode == 1 )
		{
			std::sort( rndCards.begin(),rndCards.end(),[]( const Card& a,const Card& b )
			{
				return( a.score < b.score );
			} );
		}

		for( const auto& card : rndCards )
		{
			if( card.score < reviewThresh )
			{
				if( card.score < engThresh ) jpnList.emplace_back( card );
				else engList.emplace_back( card );
				if( ++cardCount >= reviewSize )
				{
					break;
				}
			}
		}

		// std::mt19937 rng{ std::random_device{}() };
		std::shuffle( engList.begin(),engList.end(),rng );
		std::shuffle( jpnList.begin(),jpnList.end(),rng );

		std::wofstream out{ reviewPath };
		for( auto& card : jpnList )
		{
			card.MoveHint();
			out << card.japanese << "=\n";
		}
		for( auto& card : engList )
		{
			card.MoveHint();
			out << card.english << "=\n";
		}

		// const int engPos = int( float( reviewList.size() ) * engRatio );
		// 
		// for( int i = 0; i < engPos; ++i )
		// {
		// 	out << reviewList[i].japanese << '=' << '\n';
		// }
		// for( int i = engPos; i < int( reviewList.size() ); ++i )
		// {
		// 	out << reviewList[i].english << '=' << '\n';
		// }
	}

	void Grade()
	{
		auto oldReview = ReadFile( reviewPath );

		std::wofstream out{ gradePath };

		for( const auto& card : oldReview )
		{
			if( card.swapRequired ) // jpn->eng
			{
				std::vector<Card*> correctCards;
				for( auto& srcCard : cards )
				{
					// if( srcCard.japanese == card.japanese )
					if( srcCard.japanese.find( card.japanese ) != std::string::npos )
					{
						correctCards.emplace_back( &srcCard );
					}
				}

				bool correct = false;
				for( const auto& correctCard : correctCards )
				{
					// if( card.english == correctCard->english )
					if( card.english.length() > 0 &&
						correctCard->english.find( card.english ) != std::string::npos )
					{
						correct = true;
					}
				}
				if( !correct )
				{
					// card.MoveHint();
					out << card.japanese << " you put " << card.english;
					out << " -- correct answer:";
					for( const auto& correctCard : correctCards )
					{
						out << " " << correctCard->english;
					}

					for( const auto& realCard : cards )
					{
						if( realCard.japanese.find( card.japanese ) != std::string::npos )
						{
							out << realCard.GetHint();
							break;
						}
					}

					out << '\n';
				}
				for( auto& correctCard : correctCards )
				{
					correctCard->score += correct
						? correctBonus : incorrectPts;
					if( correctCard->score < 0.0f ) correctCard->score = 0.0f;
					if( correctCard->score > 1.0f ) correctCard->score = 1.0f;
				}
			}
			else
			{
				std::vector<Card*> correctCards;
				for( auto& srcCard : cards )
				{
					if( srcCard.english == card.english )
					{
						correctCards.emplace_back( &srcCard );
					}
				}

				bool correct = false;
				for( const auto& correctCard : correctCards )
				{
					// if( card.japanese == correctCard->japanese )
					if( card.japanese.length() > 0 &&
						correctCard->japanese.find( card.japanese ) != std::string::npos )
					{
						correct = true;
					}
				}
				if( !correct )
				{
					out << card.english << " you put " << card.japanese;
					out << " -- correct answer:";
					for( const auto& correctCard : correctCards )
					{
						out << " " << correctCard->japanese;
					}
					out << '\n';
				}
				for( auto& correctCard : correctCards )
				{
					correctCard->score += correct
						? correctBonus : incorrectPts;
					if( correctCard->score < 0.0f ) correctCard->score = 0.0f;
					if( correctCard->score > 1.0f ) correctCard->score = 1.0f;
				}
			}
		}
	}

	~Flashcard()
	{
		std::wofstream out{ wordBankPath };
		for( const auto& card : cards )
		{
			out << card.english << '=';
			out << card.japanese << '=';
			out << card.score << '\n';
		}
	}
private:
	std::vector<Card> ReadFile( const std::string& path ) const
	{
		std::vector<Card> curCardList;

		std::wifstream in{ path };
		std::wstring text;
		while( in.good() )
		{
			text += in.get();
		}

		std::vector<std::wstring> lines;
		lines.emplace_back( L"" );
		for( wchar_t c : text )
		{
			if( c == L'\n' ) lines.emplace_back( L"" );
			else lines.back() += c;
		}

		for( const auto& line : lines )
		{
			if( line.size() > 1 )
			{
				curCardList.emplace_back( Card{ line } );
			}
		}

		return( curCardList );
	}
private:
	static constexpr auto wordBankPath = "WordBank.txt";
	static constexpr auto reviewPath = "Review.txt";
	static constexpr auto gradePath = "Grade.txt";
	std::vector<Card> cards;
	
	// How low score must be to be included in review.
	static constexpr float reviewThresh = 0.8f;
	// How many cards require translation from eng->jpn.
	static constexpr float engRatio = 0.5f;

	static constexpr float correctBonus = 0.1f;
	static constexpr float incorrectPts = -2.0f;

	static constexpr float refreshPenalty = 0.02f;
	static constexpr float engThresh = 0.4f;

	static constexpr int reviewSize = 50;
};