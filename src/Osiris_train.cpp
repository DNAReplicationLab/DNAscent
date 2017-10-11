//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-3.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------


#include "Osiris_train.h"
#include "common.h"
#include "build_model.h"
#include "data_IO.h"
#include "error_handling.h"


static const char *help=
"NOTE: This is a special build to deal with NNBNNNN, NNNBNNN, and NNNNBNN.\n"
"train: Osiris executable that determines the mean and standard deviation of a base analogue's current.\n"
"To run Osiris train, do:\n"
"  ./Osiris train [arguments]\n"
"Example:\n"
"  ./Osiris train -r /path/to/reference.fasta -p 3and4 -bm /path/to/template_median68pA.model -d /path/to/data.foh -o output.txt -t 20 -sc 30\n"
"Required arguments are:\n"
"  -r,--reference            path to reference file in fasta format,\n"
"  -p,--position             position of analogue in training data (valid arguments are 2and3, 3and4, or 4and5),\n"
"  -om,--ont-model           path to 6mer pore model file (provided by ONT) over bases {A,T,G,C},\n"
"  -d,--trainingData         path to training data in the .foh format (can be made with Python Osiris),\n"
"  -o,--output               path to the output pore model file that Osiris will train.\n"
"Optional arguments are:\n"
"  -t,--threads              number of threads (default is 1 thread),\n"
"  -sc,--soft-clipping       restrict training to this window size around a region of interest (default is off),\n"
"  -l,--log-file             training log file for the training values at each position on the reference (default is none).\n";

struct Arguments {
	std::string referenceFilename;
	std::string analoguePosition;
	std::string trainingDataFilename;
	std::string ontModelFilename;
	std::string trainingOutputFilename;
	bool logFile;
	std::string logFilename;
	int threads;
	bool softClip;
	int SCwindow;
};

Arguments parseTrainingArguments( int argc, char** argv ){

	if( argc < 2 ){

		std::cout << "Exiting with error.  Insufficient arguments passed to Osiris train." << std::endl << help << std::endl;
		exit(EXIT_FAILURE);
	}

	if ( std::string( argv[ 1 ] ) == "-h" or std::string( argv[ 1 ] ) == "--help" ){

		std::cout << help << std::endl;
		exit(EXIT_SUCCESS);
	}
	else if( argc < 4 ){

		std::cout << "Exiting with error.  Insufficient arguments passed to Osiris train." << std::endl;
		exit(EXIT_FAILURE);
	}

	Arguments trainArgs;

	/*defaults - we'll override these if the option was specified by the user */
	trainArgs.threads = 1;
	trainArgs.softClip = false;
	trainArgs.logFile = false;

	/*parse the command line arguments */
	for ( unsigned int i = 1; i < argc; ){

		std::string flag( argv[ i ] );

		if ( flag == "-r" or flag == "--reference" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.referenceFilename = strArg;
			i+=2;	
		}
		else if ( flag == "-om" or flag == "--ont-model" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.ontModelFilename = strArg;
			i+=2;
		}
		else if ( flag == "-d" or flag == "--trainingData" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.trainingDataFilename = strArg;
			i+=2;
		}
		else if ( flag == "-o" or flag == "--output" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.trainingOutputFilename = strArg;
			i+=2;
		}
		else if ( flag == "-t" or flag == "--threads" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.threads = std::stoi( strArg.c_str() );
			i+=2;
		}
		else if ( flag == "-p" or flag == "--position" ){

			std::string strArg( argv[ i + 1 ] );
			trainArgs.analoguePosition = strArg;
			i+=2;
		}
		else if ( flag == "-sc" or flag == "--soft-clipping" ){

			trainArgs.softClip = true;
			std::string strArg( argv[ i + 1 ] );
			trainArgs.SCwindow = std::stoi( strArg.c_str() );
			i+=2;
		}
		else if ( flag == "-l" or flag == "--log-file" ){

			trainArgs.logFile = true;
			std::string strArg( argv[ i + 1 ] );
			trainArgs.logFilename = strArg;
			i+=2;
		}
		else throw InvalidOption( flag );
	}

	return trainArgs;
}


int train_main( int argc, char** argv ){

	Arguments trainArgs = parseTrainingArguments( argc, argv );

	std::string reference = import_reference( trainArgs.referenceFilename );
	std::map< std::string, std::pair< double, double > > ontModel =  import_poreModel( trainArgs.ontModelFilename );
	std::map< std::string, std::vector< std::vector< double > > > trainingData = import_foh( trainArgs.trainingDataFilename );

	std::map< std::string, std::vector< double> > trainedModel;

	int prog = 0;

	/*log file IO */
	std::ofstream logFile;
	if ( trainArgs.logFile == true ){
		logFile.open( trainArgs.logFilename );
		if ( not logFile.is_open() ){
			std::cout << "Exiting with error.  Output training log file could not be opened." << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	bool N_WarningSplashed = false; //only splash the warning for unresolved Ns once

	for( auto iter = trainingData.cbegin(); iter != trainingData.cend(); ++iter ){

		std::string refLocal = reference;
		std::vector< std::vector< double > > events = iter -> second;

		std::string adenDomain = iter -> first;
		std::string brduDomain = reverseComplement( adenDomain );

		int positionNorm;
		int adenDomLoc;
		int brduDomLoc;

		if ( trainArgs.analoguePosition == "2and3" ){
			adenDomLoc = refLocal.find( "NNNNANN" );
			brduDomLoc = refLocal.find( "NNTNNNN" );
			positionNorm = 1;
		}
		else if ( trainArgs.analoguePosition == "3and4" ){
			adenDomLoc = refLocal.find( "NNNANNN" );
			brduDomLoc = refLocal.find( "NNNTNNN" );
			positionNorm = 2;
		}
		else if ( trainArgs.analoguePosition == "4and5" ){
			adenDomLoc = refLocal.find( "NNANNNN" );
			brduDomLoc = refLocal.find( "NNNNTNN" );
			positionNorm = 3;
		}
		else{
			std::cout << "Exiting with error.  Invalid option passed with the -p or --position flag.  Valid options are 1and2, 3and4, or 5and6." << std::endl;
			exit(EXIT_FAILURE);
		}
	
		refLocal.replace( adenDomLoc, adenDomain.length(), adenDomain );
		refLocal.replace( brduDomLoc, brduDomain.length(), brduDomain );
		
		/*check for unresolved Ns and warn if there are any */
		if ( not N_WarningSplashed ){
			for ( auto it = refLocal.begin(); it < refLocal.end(); it++ ){
				if ( *it == 'N' ){
					std::cout << "WARNING: reference contains unresolved Ns.  Did you mean to do this?" << std::endl;
					N_WarningSplashed = true;
					break;
				}
			}
		}

		/*if soft clipping was specified, truncate the reference and events with dynamic time warping */
		if ( trainArgs.softClip == true ){

			if ( ( brduDomLoc - trainArgs.SCwindow < 0 ) or ( brduDomLoc + trainArgs.SCwindow > refLocal.length() ) ){

				std::cout << "Exiting with error.  Soft clipping window exceeds reference length.  Reduce window size." << std::endl;
				exit(EXIT_FAILURE);
			}

			refLocal = refLocal.substr( brduDomLoc - trainArgs.SCwindow, 6 + 2*trainArgs.SCwindow );
			brduDomLoc = trainArgs.SCwindow;
			events = filterEvents( refLocal, ontModel, events );
		}

		/*do the training */
		std::stringstream ss = buildAndTrainHMM( refLocal, ontModel, events, trainArgs.threads, false );

		/*if we specified that we want a log file, read the ss buffer into it now */
		std::stringstream ssLog( ss.str() );
		if ( trainArgs.logFile == true ){
			logFile << ">" << adenDomain << std::endl << ssLog.rdbuf();
		}

		/*hacky bodge to get the training data out at the relevant position without making Penthus specialised */
		std::string line;
		while ( std::getline( ss, line ) ){
			
			std::vector< std::string > findIndex = split( line, '_' );
			unsigned int i = atoi(findIndex[0].c_str());

			if ( i == brduDomLoc ){

				std::string atFirstPos = ( brduDomain.substr( 0, 6 ) ).replace( positionNorm + 1, 1, "B" );
				std::vector< std::string > splitLine = split( line, '\t' );
				
				if ( trainedModel.count( atFirstPos ) > 0 ){

					if ( atof( splitLine[ 5 ].c_str() ) < trainedModel[ atFirstPos ][ 1 ] ){

						trainedModel[ atFirstPos ] = { atof( splitLine[ 3 ].c_str() ), atof( splitLine[ 5 ].c_str() ), atof( splitLine[ 2 ].c_str() ), atof( splitLine[ 4 ].c_str() ) };
					}
				}
				else{

					trainedModel[ atFirstPos ] = { atof( splitLine[ 3 ].c_str() ), atof( splitLine[ 5 ].c_str() ), atof( splitLine[ 2 ].c_str() ), atof( splitLine[ 4 ].c_str() ) };
				}

			}
			else if ( i == brduDomLoc + 1 ){

				std::string atSecondPos = ( brduDomain.substr( 1, 6 ) ).replace( positionNorm, 1, "B" );
				std::vector< std::string > splitLine = split( line, '\t' );

				if ( trainedModel.count( atSecondPos ) > 0 ){

					if ( atof( splitLine[ 5 ].c_str() ) < trainedModel[ atSecondPos ][ 1 ] ){

						trainedModel[ atSecondPos ] = { atof( splitLine[ 3 ].c_str() ), atof( splitLine[ 5 ].c_str() ), atof( splitLine[ 2 ].c_str() ), atof( splitLine[ 4 ].c_str() ) };
					}
				}
				else{

					trainedModel[ atSecondPos ] = { atof( splitLine[ 3 ].c_str() ), atof( splitLine[ 5 ].c_str() ), atof( splitLine[ 2 ].c_str() ), atof( splitLine[ 4 ].c_str() ) };
				}
			}
			else if ( i > brduDomLoc + 1 ){
		
				break;
			}
		}
		displayProgress( prog, trainingData.size() );
		prog++;
	}

	/*make a pore model file from the map */
	export_poreModel( trainedModel, trainArgs.trainingOutputFilename );

	/*if we opened a log file to write on, close it now */
	if ( trainArgs.logFile == true ){
		logFile.close();
	}

	/*some wrap-up messages */
	std::cout << std::endl << "Done." << std::endl;

	return 0;

}
