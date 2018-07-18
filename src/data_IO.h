//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-3.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef DATA_IO_H
#define DATA_IO_H


#include <string>
#include <map>


struct read{

	std::string basecall, referenceSeqMappedTo, referenceMappedTo, filename, readID;
	std::vector< double > raw, normalisedEvents;
	std::map< int, int > refToQuery;
	std::vector< std::pair< unsigned int, unsigned int > > eventAlignment;
	double qualityScore;
	int refStart, refEnd;
};


/*function prototypes */
std::map< std::string, std::string > import_reference( std::string );
std::map< std::string, std::pair< double, double > > import_poreModel( std::string );
std::pair< std::string, std::vector< read > > getTrainingFrom_foh( std::string &);
read getDetectionFrom_fdh( std::string & );
void export_poreModel( std::map< std::string, std::vector< std::pair< double, double > > > &, std::string &);

#endif
