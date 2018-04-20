//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-3.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------


#ifndef COMMON_H
#define COMMON_H


#include <algorithm>
#include <vector>
#include <utility>
#include <iterator>
#include <map>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>


class progressBar{

	private:
		std::chrono::time_point<std::chrono::steady_clock> _startTime;
		std::chrono::time_point<std::chrono::steady_clock> _currentTime;
		unsigned int maxNumber;
		unsigned int barWidth = 70;
		unsigned int _digits;

	public:
		progressBar( unsigned int maxNumber ){
		
			this -> maxNumber = maxNumber;
			_digits = std::to_string( maxNumber).length() + 1;
			_startTime = std::chrono::steady_clock::now();
		}
		void displayProgress( unsigned int currentNumber, unsigned int failed ){

			_currentTime = std::chrono::steady_clock::now();
			 std::chrono::duration<double> elapsedTime = _currentTime - _startTime;

			double progress = (double) currentNumber / (double) maxNumber;
			
			if ( progress <= 1.0 ){

				std::cout << "[";
				int pos = barWidth * progress;
				for (int i = 0; i < barWidth; ++i) {
					if (i < pos) std::cout << "=";
					else if (i == pos) std::cout << ">";
					else std::cout << " ";
				}
				std::cout << "] " << std::right << std::setw(3) << int(progress * 100.0) << "%  ";

				std::cout << std::right << std::setw(_digits) << currentNumber << "/" << maxNumber << "  ";


				unsigned int estTimeLeft = elapsedTime.count() * ( (double) maxNumber / (double) currentNumber - 1.0 );			
				unsigned int hours = estTimeLeft / 3600;
				unsigned int mins = (estTimeLeft % 3600) / 60;
				unsigned int secs = (estTimeLeft % 3600) % 60;

				std::cout << std::right << std::setw(2) << hours << "hr" << std::setw(2) << mins << "min" << std::setw(2) << secs << "sec  ";
				std::cout << "f: " << std::right << std::setw(_digits) << failed << std::setw(3) << "\r";
				std::cout.flush();

			}
		} 
};


inline std::string reverseComplement( std::string DNAseq ){

	std::reverse( DNAseq.begin(), DNAseq.end() );
	std::string revComp;

	for ( std::string::iterator i = DNAseq.begin(); i < DNAseq.end(); i++ ){
	
		switch( *i ){
			case 'A' :
				revComp += 'T';
				break;
			case 'T' :
				revComp += 'A';
				break;
			case 'G' :
				revComp += 'C';
				break;
			case 'C' :
				revComp += 'G';
				break;
			default:
				std::cout << "Exiting with error.  Invalid character passed to reverse complement function.  Must be A, T, G, or C." << std::endl;
				exit( EXIT_FAILURE );
		}
	}
	return revComp;
}


/*function prototypes */
void displayProgress( int, int );
std::vector< std::string > split( std::string, char );
int argMin( std::vector< double > & );


#endif
