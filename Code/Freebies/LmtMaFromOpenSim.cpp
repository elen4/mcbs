/*  
 * Copyright (C) 2013 C. Pizzolato, M. Reggiani, M. Sartori
 * 
 * This file is part of Multidimensional Cubic Bspline Library (MCBS).
 * 
 * MCBS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MCBS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MCBS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LmtMaFromOpenSim.h"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ofstream;
#include <stdlib.h>
#include <boost/concept_check.hpp>
#include <fstream>
using std::ostream;
#include <OpenSim/OpenSim.h>


#include "OpenSim2StdTools.h"



const int NUMBER_DIGIT_OUTPUT = 15;


inline double radians (double d) {
return d * M_PI / 180;
}

inline double degrees (double r) {
return r * 180/ M_PI;
}

LmtMaFromOpenSim::LmtMaFromOpenSim(){
    cout << "LmtMaFromOpenSim default constructor: you should not be here!";
    exit(EXIT_FAILURE);
}

LmtMaFromOpenSim::LmtMaFromOpenSim(const string& osimModelFilename, const vector<string>& dofNames, const vector<string>& musclesNames, const vector< vector<double> >& angleCombinations) {
    osimModel_ = new OpenSim::Model(osimModelFilename);
    SimTK::State& stateMuscleAnalysis = osimModel_->initSystem(); //no idea why I have to do this
    osimModel_->getMultibodySystem().realize(stateMuscleAnalysis, SimTK::Stage::Position); //and this, I guess it is just an initialization
    
    computeLmtMaFromOpenSim(dofNames, musclesNames, angleCombinations);
}


LmtMaFromOpenSim::~LmtMaFromOpenSim(){

}

LmtMaFromOpenSim::LmtMaFromOpenSim(const LmtMaFromOpenSim& orig) {
    // *this = orig; NON SONO SICURA CHE I costruttori di copia siano fatti decenetemten
    // in opensim... evitiamo... :)
    cout << "LmtMaFromOpenSim: copy constructor: Should not be here!!!\n";
    exit(EXIT_FAILURE);
}

void LmtMaFromOpenSim::computeLmtMaFromOpenSim(const vector<string>& dofNames, const vector<string>& musclesNames, const vector< vector<double> >& angleCombinations) {
    
    // clear data from previous analysis
    osimMusclesNames_.clear();
    osimDofNames_.clear();
    lmtData_.clear();
    anglesDataForModel_.purge();    
    
    for(int i = 0; i < musclesNames.size(); ++i) {
        string osimName = OpenSim2StdTools::std2osimMuscleName(musclesNames.at(i));
        osimMusclesNames_.push_back(osimName);
    }
    
    nDofs_ = dofNames.size();
    for(int i = 0; i < nDofs_; ++i) {
        string osimName = OpenSim2StdTools::std2osimDofName(dofNames.at(i));
        osimDofNames_.push_back(osimName);
    }
    
    convertToStorage(angleCombinations);    
    runMuscleAnalysis();
    
}

void LmtMaFromOpenSim::convertToStorage(const vector<vector<double> >& anglesCombinations) {
    
    startTime_ = .0;
    endTime_ = anglesCombinations.at(0).size()-1;

    anglesDataForModel_.setInDegrees(false);
    double currentRow[anglesCombinations.size()];
    int nRows = anglesCombinations.at(0).size();
    for(int iRow = 0; iRow < nRows; ++iRow) {
        OpenSim::StateVector currentState;
        for(int iCol = 0; iCol < anglesCombinations.size(); ++iCol)
            currentRow[iCol] = radians(anglesCombinations.at(iCol).at(iRow));
        currentState.setStates(iRow, anglesCombinations.size(), currentRow); 
        anglesDataForModel_.append(currentState);
    }
    OpenSim::Array<string> columnLabels;
    columnLabels.append("time");
    for(int i = nDofs_-1; i >= 0; --i) {
        columnLabels.append(osimDofNames_.at(i));
    }    
    anglesDataForModel_.setColumnLabels(columnLabels);
}


void LmtMaFromOpenSim::runMuscleAnalysis() {
    OpenSim::Array<std::string> muscles;
    OpenSim2StdTools::vector2array(osimMusclesNames_, muscles);

    OpenSim::MuscleAnalysis *muscleTendonLengthAnalysis = new OpenSim::MuscleAnalysis(osimModel_); 
    muscleTendonLengthAnalysis->setMuscles(muscles);
    muscleTendonLengthAnalysis->setStartTime(startTime_);
    muscleTendonLengthAnalysis->setEndTime(endTime_); 
    muscleTendonLengthAnalysis->setComputeMoments(false);
    muscleTendonLengthAnalysis->setPrintResultFiles(false);
    //muscleTendonLengthAnalysis->setName("test");  
    
    //add analysis to model and update the model state
    osimModel_->addAnalysis(muscleTendonLengthAnalysis);
    SimTK::State& stateMuscleAnalysis2 = osimModel_->initSystem();
    osimModel_->getMultibodySystem().realize(stateMuscleAnalysis2, SimTK::Stage::Position);

    //create an AnalyzeTool with a passed in OpenSim Model (a model with the muscle analysis)
    OpenSim::AnalyzeTool musclePlotterTool(osimModel_[0]);
    //create a storage from a motion file
    //load motion file into the AnalyzeTool
    musclePlotterTool.setStatesFromMotion(stateMuscleAnalysis2, anglesDataForModel_, true);

    //add analysis to the model and update model's state
    musclePlotterTool.addAnalysisSetToModel();
    SimTK::State& stateMuscleAnalysis3 = osimModel_->initSystem();
    osimModel_->getMultibodySystem().realize(stateMuscleAnalysis3, SimTK::Stage::Position);
    musclePlotterTool.setInitialTime(startTime_);
    musclePlotterTool.setFinalTime(endTime_);
    musclePlotterTool.setResultsDir("MuscleAnalysis");                
    musclePlotterTool.setStatesFileName("MuscleAnalysis/test");
    musclePlotterTool.run();
    
    OpenSim::Storage* myLmtStorage;
    myLmtStorage = muscleTendonLengthAnalysis->getMuscleTendonLengthStorage();
    int nRows = myLmtStorage->getSize();
    
    for(int iCol = 0; iCol < osimMusclesNames_.size(); ++iCol){    
        double* tempCol = new double[nRows];
        myLmtStorage->getDataColumn(iCol,tempCol);
        vector<double> column;
        for(int k = 0; k < nRows; ++k)
            column.push_back(*(tempCol+k));
        lmtData_.push_back(column);
        delete[] tempCol;    
    }
    
    delete muscleTendonLengthAnalysis;
  

    const OpenSim::ArrayPtrs<OpenSim::MuscleAnalysis::StorageCoordinatePair>& myMaArraysOfStorage = muscleTendonLengthAnalysis->getMomentArmStorageArray();
    int numCoordinates = myMaArraysOfStorage.getSize();
    
    
    //:MONICA: From here... this is not working
      for (int coordinateIterator = 0; coordinateIterator < numCoordinates; ++coordinateIterator) {
        
        string currentCoordinateName = myMaArraysOfStorage[coordinateIterator]->q->getName();
        if ( find(osimDofNames_.begin(), osimDofNames_.end(), currentCoordinateName ) != osimDofNames_.end() ) { 
          osimCoordinateNames_.push_back(currentCoordinateName);
          vector< vector< double > > currentCoordinateMomentArm;
          for(int iCol = 0; iCol < osimMusclesNames_.size(); ++iCol){
            double* tempCol = new double[nRows];
            myMaArraysOfStorage[coordinateIterator]->momentStore->getDataColumn(iCol,tempCol);
            vector<double> column;
            for(int k = 0; k < nRows; ++k)
              column.push_back(*(tempCol+k));
            currentCoordinateMomentArm.push_back(column);
            delete[] tempCol;    
          }
          maData_.push_back(currentCoordinateMomentArm);
        }
      }
    
    
}


void LmtMaFromOpenSim::saveLmt(ostream &os) {
    for(int i = 0; i < osimMusclesNames_.size()-1; ++i)
        os << OpenSim2StdTools::osim2stdMuscleName(osimMusclesNames_.at(i)) << " ";
    // the last one is printed outside the for cycle because we do not want " " at the end of the line
    os << OpenSim2StdTools::osim2stdMuscleName(osimMusclesNames_.back()) << endl; 
    int nRows = lmtData_.at(1).size();
    int nCols = lmtData_.size();
    for(int iRow = 0; iRow < nRows; ++iRow) {
        for(int iCol = 0; iCol < nCols; ++iCol)
            os << lmtData_.at(iCol).at(iRow) << "\t";
        os << endl;
    }   
}

void LmtMaFromOpenSim::saveMa(const string& outputDirectory) {
  cout << "saving MA" << endl; 
  for (int coordinateIterator = 0; coordinateIterator < osimCoordinateNames_.size(); ++coordinateIterator) {
    // open next ma file
    string maFileName = outputDirectory + "ma" + OpenSim2StdTools::osim2stdDofName(osimCoordinateNames_.at(coordinateIterator)) + ".out"; 
    cout << maFileName;
    ofstream maFile(maFileName.c_str());
    for(int i = 0; i < osimMusclesNames_.size()-1; ++i)
        maFile << OpenSim2StdTools::osim2stdMuscleName(osimMusclesNames_.at(i)) << " ";
    // the last one is printed outside the for cycle because we do not want " " at the end of the line
    maFile << OpenSim2StdTools::osim2stdMuscleName(osimMusclesNames_.back()) << endl; 
    int nRows = maData_.at(coordinateIterator).at(1).size();
    int nCols = maData_.at(coordinateIterator).size();
    for(int iRow = 0; iRow < nRows; ++iRow) {
        for(int iCol = 0; iCol < nCols; ++iCol)
            maFile << maData_.at(coordinateIterator).at(iCol).at(iRow) << "\t";
        maFile << endl;
    }  
    
    maFile.close();
    
  }
   
  
  
}



  
  
  
  
  
    