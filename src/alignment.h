//----------------------------------------------------------
// Copyright 2019-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-3.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef ALIGN_H
#define ALIGN_H

#include <fstream>
#include "detect.h"
#include <math.h>
#include <stdlib.h>
#include <limits>
#include "common.h"
#include "event_handling.h"
#include "../fast5/include/fast5.hpp"
#include "poreModels.h"
#include "common.h"
#include <memory>
#include <utility>

#define NFEATURES 8


class AlignedPosition{

	private:

		bool forTraining = false;
		std::string sixMer;
		unsigned int refPos;
		std::vector<double> events;
		std::vector<double> lengths;

	public:
		AlignedPosition(std::string sixMer, unsigned int refPos){

			this -> sixMer = sixMer;
			this -> refPos = refPos;
		}
		~AlignedPosition() {};
		void addEvent(double ev, double len){

			events.push_back(ev);
			lengths.push_back(len);
		}
		std::string getSixMer(void){

			return sixMer;
		}
		unsigned int getRefPos(void){

			return refPos;
		}
		std::vector<double> makeFeature(void){

			assert(events.size() > 0 && events.size() == lengths.size());
			assert(sixMer.substr(0,1) == "A" || sixMer.substr(0,1) == "T" || sixMer.substr(0,1) == "G" || sixMer.substr(0,1) == "C");

			//one-hot encode bases
			std::vector<double> feature = {0., 0., 0., 0.};
			if (sixMer.substr(0,1) == "A") feature[0] = 1.;
			else if (sixMer.substr(0,1) == "T") feature[1] = 1.;
			else if (sixMer.substr(0,1) == "G") feature[2] = 1.;
			else if (sixMer.substr(0,1) == "C") feature[3] = 1.;

			//events
			double eventMean = vectorMean(events);
			feature.push_back(eventMean);

			//event lengths
			double lengthsSum = vectorSum(lengths);
			feature.push_back(lengthsSum);

			//pore model
			std::pair<double,double> meanStd = thymidineModel[sixMer2index(sixMer)];
			feature.push_back(meanStd.first);
			feature.push_back(meanStd.second);

			return feature;
		}
};


class AlignedRead{

	private:
		std::string readID, chromosome, strand;
		std::map<unsigned int, std::shared_ptr<AlignedPosition>> positions;
		unsigned int mappingLower, mappingUpper;

	public:
		AlignedRead(std::string readID, std::string chromosome, std::string strand, unsigned int ml, unsigned int mu, unsigned int numEvents){

			this -> readID = readID;
			this -> chromosome = chromosome;
			this -> strand = strand;
			this -> mappingLower = ml;
			this -> mappingUpper = mu;
		}
		AlignedRead( const AlignedRead &ar ){

			this -> readID = ar.readID;
			this -> chromosome = ar.chromosome;
			this -> strand = ar.strand;
			this -> mappingLower = ar.mappingLower;
			this -> mappingUpper = ar.mappingUpper;
			this -> positions = ar.positions;
		}
		~AlignedRead(){}
		void addEvent(std::string sixMer, unsigned int refPos, double ev, double len){

			if (positions.count(refPos) == 0){

				std::shared_ptr<AlignedPosition> ap( new AlignedPosition(sixMer, refPos));
				ap -> addEvent(ev,len);
				positions[refPos] = ap;
			}
			else{

				positions[refPos] -> addEvent(ev,len);
			}
		}
		std::string getReadID(void){
			return readID;
		}
		std::string getChromosome(void){
			return chromosome;
		}
		std::string getStrand(void){
			return strand;
		}
		unsigned int getMappingLower(void){
			return mappingLower;
		}
		unsigned int getMappingUpper(void){
			return mappingUpper;
		}
		std::vector<double> makeTensor(void){

			assert(strand == "fwd" || strand == "rev");
			std::vector<double> tensor;
			tensor.reserve(NFEATURES * positions.size());

			if (strand == "fwd"){

				for (auto p = positions.begin(); p != positions.end(); p++){

					std::vector<double> feature = (p -> second) -> makeFeature();
					tensor.insert(tensor.end(), feature.begin(), feature.end());
				}
			}
			else{

				for (auto p = positions.rbegin(); p != positions.rend(); p++){

					std::vector<double> feature = (p -> second) -> makeFeature();
					tensor.insert(tensor.end(), feature.begin(), feature.end());
				}
			}
			return tensor;
		}
		std::vector<unsigned int> getPositions(void){

			std::vector<unsigned int> out;
			out.reserve(positions.size());
			if (strand == "fwd"){

				for (auto p = positions.begin(); p != positions.end(); p++){
					out.push_back(p -> first);
				}
			}
			else{

				for (auto p = positions.rbegin(); p != positions.rend(); p++){
					out.push_back(p -> first);
				}
			}
			return out;
		}
		std::vector<std::string> getSixMers(void){

			std::vector<std::string> out;
			out.reserve(positions.size());
			if (strand == "fwd"){

				for (auto p = positions.begin(); p != positions.end(); p++){
					out.push_back((p -> second) -> getSixMer());
				}
			}
			else{

				for (auto p = positions.rbegin(); p != positions.rend(); p++){
					out.push_back((p -> second) -> getSixMer());
				}
			}
			return out;
		}
		std::pair<size_t, size_t> getShape(void){

			return std::make_pair(positions.size(), NFEATURES);
		}
};


/*function prototypes */
int align_main( int argc, char** argv );
std::string eventalign_train( read &, unsigned int , std::map<unsigned int, double> &, double);
std::pair<bool,std::shared_ptr<AlignedRead>> eventalign_detect( read &, unsigned int, double );

#endif
