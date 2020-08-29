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
			if( isalpha( japanese[0] ) || !isalpha( english[0] ) )
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
		// TODO: update score based on date dt

	}

	void GenerateReview()
	{
		std::vector<Card> reviewList;
		for( const auto& card : cards )
		{
			if( card.score < reviewThresh )
			{
				reviewList.emplace_back( card );
			}
		}

		std::mt19937 rng{ std::random_device{}() };
		std::shuffle( reviewList.begin(),reviewList.end(),rng );

		std::wofstream out{ reviewPath };
		const int engPos = int( float( reviewList.size() ) * engRatio );

		for( int i = 0; i < engPos; ++i )
		{
			out << reviewList[i].japanese << '=' << '\n';
		}
		for( int i = engPos; i < int( reviewList.size() ); ++i )
		{
			out << reviewList[i].english << '=' << '\n';
		}
	}

	void Grade()
	{
		const auto oldReview = ReadFile( reviewPath );

		std::wofstream out{ gradePath };

		for( const auto& card : oldReview )
		{
			if( card.swapRequired ) // jpn->eng
			{
				std::vector<Card*> correctCards;
				for( auto& srcCard : cards )
				{
					if( srcCard.japanese == card.japanese )
					{
						correctCards.emplace_back( &srcCard );
					}
				}

				bool correct = false;
				for( const auto& correctCard : correctCards )
				{
					if( card.english == correctCard->english )
					{
						correct = true;
					}
				}
				if( !correct )
				{
					out << card.japanese << " you put " << card.english;
					out << " -- correct answer:";
					for( const auto& correctCard : correctCards )
					{
						out << " " << correctCard->english;
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
	std::vector<Card> ReadFile( const std::string& path )
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

	static constexpr float correctBonus = 0.5f;
	static constexpr float incorrectPts = -0.7f;
};