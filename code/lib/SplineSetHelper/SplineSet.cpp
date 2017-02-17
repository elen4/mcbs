/*
 * Copyright (C) 2013 C. Pizzolato, M. Reggiani, M. Sartori
 * Copyright (C) 2015 E.Ceseracciu
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


#include "mcbs/SplineSet.h"
#include <mcbs/Spline.cpp>

#include <cstdlib>
#include <iostream>
using std::cout;
#include <fstream>
using std::ifstream;
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::stringstream;
#include <algorithm>
using std::find;
#include <iostream>
using std::endl;
#include <iomanip>
#include <map>
using std::map;
#include <fstream>
using std::ifstream;
using std::ofstream;

namespace mcbs {

    template <int N_DOF_SPLINESET>
    SplineSet<N_DOF_SPLINESET>::SplineSet(const string& inputDataFilename)
        :dofNames_(N_DOF_SPLINESET), a_(N_DOF_SPLINESET), b_(N_DOF_SPLINESET), n_(N_DOF_SPLINESET), inputDataFile_(inputDataFilename.c_str()) {

        if (!inputDataFile_.is_open()) {
            cout << "ERROR: " << inputDataFilename << " could not be open\n";
            exit(EXIT_FAILURE);
        }

#ifdef LOG
        cout << "Reading input data from: "
            << inputDataFilename << endl;
#endif

        readInputData();
        inputDataFile_.close();

#ifdef LOG
        cout << "Read the following interpolation data:\n";
        displayInputData();
#endif

        // create the noMuscles_ splines
        for (int i = 0; i < noMuscles_; ++i) {
            Spline<N_DOF_SPLINESET> newSpline(a_, b_, n_);
            splines_.push_back(newSpline);
        }

#ifdef LOG
        cout << "Created " << splines_.size() << " splines.\n";
#endif

        // now comtemplate <int N_DOF_SPLINESET>pute coefficients for each muscle
        for (int i = 0; i < noMuscles_; ++i) {
            vector<double> currentMuscle(y_[i]);
            splines_[i].computeCoefficients(currentMuscle, currentMuscle.begin());
        }

    }

    template <int N_DOF_SPLINESET>
    SplineSet<N_DOF_SPLINESET>::SplineSet(const std::vector< std::string > dofNames, const std::vector< std::string > musclesNames, const std::vector<double>& a, const std::vector<double>& b, const std::vector<int>& n, const std::vector<std::vector<double> >& y)
    {
        muscleNames_ = musclesNames;
        noMuscles_ = musclesNames.size();
        dofNames_ = dofNames;
        a_ = a; //todo: check that first dim in N_DOF
        b_ = b;
        n_ = n;
        y_ = y;

        // create the noMuscles_ splines
        for (int i = 0; i < noMuscles_; ++i) {
            Spline<N_DOF_SPLINESET> newSpline(a_, b_, n_);
            splines_.push_back(newSpline);
        }

        // now compute coefficients for each muscle
        for (int i = 0; i < noMuscles_; ++i) {
            vector<double> currentMuscle(y_[i]);
            splines_[i].computeCoefficients(currentMuscle, currentMuscle.begin());
        }


    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::readInputData() {

        string dummy;
        int nDofs;
        inputDataFile_ >> dummy; // TODO check that dummy is "ndof"
        inputDataFile_ >> nDofs; //TODO check that it matches N_DOF_SPLINESET
        // --- Read DOFs
        for (int i = 0; i < N_DOF_SPLINESET; ++i) {
            inputDataFile_ >> dofNames_[i];
            inputDataFile_ >> a_[i]; //a_[i] = radians(a_[i]);
            inputDataFile_ >> b_[i]; //b_[i] = radians(b_[i]);
            inputDataFile_ >> n_[i];
        }

        string line;
        getline(inputDataFile_, line, '\n'); getline(inputDataFile_, line, '\n');
        stringstream myStream(line);
        string nextMuscleName;
        // --- Read Interpolation Data
        // 1. first their names
        do {
            myStream >> nextMuscleName;
            muscleNames_.push_back(nextMuscleName);
        } while (!myStream.eof());

        noMuscles_ = muscleNames_.size();

        // 2. then their values for all the possible combination of DOFs values
        // 2a. create the matrix to store them
        noInputData_ = 1;
        for (int i = 0; i < N_DOF_SPLINESET; ++i)
            noInputData_ *= (n_[i] + 1);
        for (int i = 0; i < noMuscles_; ++i)
            y_.push_back(vector<double>(noInputData_));

        // 2b. read the data
        for (int j = 0; j < noInputData_; ++j)
            for (int i = 0; i < noMuscles_; ++i) {
                inputDataFile_ >> y_[i][j];
            }

    }

    template <int N_DOF_SPLINESET>
    SplineSet<N_DOF_SPLINESET>::SplineSet(const string& coeffDir, const vector<string>& muscleNames)
        :dofNames_(N_DOF_SPLINESET), a_(N_DOF_SPLINESET), b_(N_DOF_SPLINESET), n_(N_DOF_SPLINESET), muscleNames_(muscleNames) {
        string anglesFilename = coeffDir + "/dofAngles.cfg";
        ifstream anglesFile(anglesFilename.c_str());
        if (!anglesFile.is_open()) {
            cout << "ERROR: " << anglesFilename << " could not be open\n";
            exit(EXIT_FAILURE);
        }

        string trash;
        anglesFile >> trash;

        int noDofs;
        anglesFile >> noDofs;

        if (noDofs != N_DOF_SPLINESET)
        {
            std::cout << "ERROR: You are trying to use coefficients created for a " <<
                noDofs << "-dimensional spline to create a " << N_DOF_SPLINESET
                << "-dimensional spline!" << std::endl;
            exit(EXIT_FAILURE);
        }


        for (int i = 0; i < N_DOF_SPLINESET; ++i) {
            anglesFile >> dofNames_[i];
            anglesFile >> a_[i]; //a_[i] = radians(a_[i]);
            anglesFile >> b_[i]; //b_[i] = radians(b_[i]);
            anglesFile >> n_[i];
        }

        anglesFile.close();

#ifdef LOG
        cout << "Created " << splines_.size() << " splines.\n";
#endif

        noMuscles_ = muscleNames_.size();
        // create the noMuscles_ splines
        for (int i = 0; i < noMuscles_; ++i) {
            Spline<N_DOF_SPLINESET> newSpline(a_, b_, n_);
            splines_.push_back(newSpline);
        }

#ifdef LOG
        cout << "Created " << splines_.size() << " splines.\n";
#endif


#ifdef LOG
        cout << "Set up spline with coefficients:\n";
#endif
        for (int i = 0; i < noMuscles_; ++i) {
            vector<double> coefficients;
            getCoefficientsFromFile(coefficients, coeffDir, muscleNames_.at(i));
            splines_.at(i).setCoefficients(coefficients);
        }
    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::getCoefficientsFromFile(vector<double>& coefficientsFromFile, const string& coeffDir, const string& muscleName) {

        string coeffFilename = coeffDir + "/" + muscleName + ".bin";
        std::fstream inputCoeffFile(coeffFilename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
        if (!inputCoeffFile.is_open()) {
            cout << "ERROR: " << coeffFilename << " could not be open\n";
            exit(EXIT_FAILURE);
        }

        cout << "Using " << muscleName << ".bin as coefficients file\n";
        std::ifstream::pos_type dataSize;
        dataSize = inputCoeffFile.tellg();
        char* dataFromFile;
        dataFromFile = new char[dataSize];
        inputCoeffFile.seekg(0, std::ios::beg);
        inputCoeffFile.read(dataFromFile, dataSize);
        inputCoeffFile.close();
        double* ptr = (double*)dataFromFile;
        do{
            coefficientsFromFile.push_back(*(ptr++));
        } while (ptr < (double*)(dataFromFile + dataSize));
        delete[] dataFromFile;
    }


    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::exportCoefficients(const string& outDir){

        for (unsigned int i = 0; i < muscleNames_.size(); ++i) {
            string outputFilename = outDir + muscleNames_.at(i) + ".bin";
            std::fstream outputCoeffFile(outputFilename.c_str(), std::ios::out | std::ios::binary);
            if (!outputCoeffFile.is_open()) {
                cout << "ERROR: " + outputFilename + "File could not be open\n";
                exit(EXIT_FAILURE);
            }
            vector<double> coefficients = splines_.at(i).getCoefficients();
            for (unsigned int j = 0; j < coefficients.size(); ++j) {
                double c = coefficients.at(j);
                outputCoeffFile.write((const char*)&c, sizeof(c));
            }
            outputCoeffFile.close();
        }
    }

    /**
     * return the lmtValues for the set of muscles when
     * a set of dof values are received in input
     */
    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::getLmt(vector<double>& lmtValues, const vector<double>& dofValues) {
        lmtValues.clear();
        for (unsigned int currentMuscle = 0; currentMuscle < muscleNames_.size(); ++currentMuscle)
        {
            try
            {
                lmtValues.push_back(splines_.at(currentMuscle).getValue(dofValues));
            }
            catch (mcbs::x_out_of_bounds &e)
            {
                throw e;
            }
        }
    }

    /**
     * store in files the values of Ma
     */


    inline double roundIt(double x, double n) {
        return floor(x * pow(10.0, n) + 0.5) / pow(10.0, n);
    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::evalLmt(const string& outputDir, const vector< vector< double > >& angleCombinations) {

        string lmtFileName = outputDir + "/lmt.out";
        ofstream lmtFile(lmtFileName.c_str());
        if (!lmtFile.is_open()) {
            cout << "ERROR: " << lmtFileName << " output file could not be open\n";
            exit(EXIT_FAILURE);
        }

        for (unsigned int i = 0; i < muscleNames_.size(); ++i)
            lmtFile << muscleNames_.at(i) << "\t";
        lmtFile << endl;

        int noCombinations = angleCombinations.size();
        for (int i = 0; i < noCombinations; ++i) {
            vector<double> lmtValues;
            for (unsigned int currentMuscle = 0; currentMuscle < muscleNames_.size(); ++currentMuscle)
                lmtValues.push_back(splines_.at(currentMuscle).getValue(angleCombinations.at(i)));
            for (unsigned int j = 0; j < muscleNames_.size(); ++j)
                lmtFile << lmtValues.at(j) << "\t";
            lmtFile << endl;
        }
        lmtFile.close();
    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::getMa(const string& dofName,
        const vector< string >& musclesConnectedToDofs,
        const vector< double >& angleCombinations,
        vector <double>& maValues) {

        bool found = false;
        size_t k = 0;
        int dofIndex;
        while ((!found) && (k < dofNames_.size())) {
            if (dofName == dofNames_.at(k)) {
                dofIndex = k;
                found = true;
            }
            k++;
        }

        if (!found) {
            cout << "ERROR: " << dofName << " not found\n";
            exit(EXIT_FAILURE); //TODO is exit appropriate?
        }

        maValues.resize(musclesConnectedToDofs.size());
        for (unsigned int i = 0; i < musclesConnectedToDofs.size(); ++i) {
            vector<string>::iterator iter = find(muscleNames_.begin(), muscleNames_.end(), musclesConnectedToDofs.at(i));
            if (iter == muscleNames_.end()) {
                cout << "something went wrong.. muscle not found in Splines \n";
                exit(EXIT_FAILURE); //TODO is exit appropriate?
            }
            size_t splineNum = std::distance(muscleNames_.begin(), iter);

            try
            {
                maValues.at(i) = -splines_[splineNum].getFirstDerivative(angleCombinations, dofIndex);
            }
            catch (mcbs::x_out_of_bounds& e)
            {
                throw e;
            }
        }

    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::getMaDerivative( const string& dofName,
                           const vector< string >& musclesConnectedToDofs,
                           const vector< double >& angleCombinations,
                           vector <double>& maDerivativeValues  ) {

      bool found = false;
      int k = 0;
      int dofIndex;
      while ((!found) && (k < dofNames_.size()) ) {
        if (dofName == dofNames_.at(k)) {
          dofIndex = k;
          found = true;
        }
        k++;
      }

      if (!found) {
        cout << "ERROR: " << dofName << " not found\n";
        exit(EXIT_FAILURE); //TODO is exit appropriate?
      }

      maDerivativeValues.resize(musclesConnectedToDofs.size());
      for (unsigned int i = 0; i < musclesConnectedToDofs.size(); ++i) {
        vector<string>::iterator iter = find(muscleNames_.begin(), muscleNames_.end(), musclesConnectedToDofs.at(i));
        if (iter == muscleNames_.end()) {
          cout << "something went wrong.. muscle not found in Splines \n";
          exit(EXIT_FAILURE); //TODO is exit appropriate?
        }
        size_t splineNum = std::distance(muscleNames_.begin(), iter);

        try
        {
            maDerivativeValues.at(i) = -splines_[splineNum].getSecondDerivative(angleCombinations,dofIndex);
        }
        catch (mcbs::x_out_of_bounds& e)
        {
            throw e;
        }
      }

    }

    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::evalMa(const string& outputDir, const  map<string, vector<string> >& musclesConnectedToDofs, const vector< vector< double > >& angleCombinations  ) {

        const int DIGIT_NUM = 8;
        const int NUMBER_DIGIT_OUTPUT = 8;



        // for all the degrees of freedom in the map
        for (map<string, vector<string> >::const_iterator dofMusclesIT = musclesConnectedToDofs.begin(); dofMusclesIT != musclesConnectedToDofs.end(); ++dofMusclesIT) {
            vector<string>::iterator dofIt = find(dofNames_.begin(), dofNames_.end(), dofMusclesIT->first);
            if (dofIt == dofNames_.end()) {
                cout << "ERROR: dofsNames differ\n";
                exit(EXIT_FAILURE);
            }
            size_t dofIndex = dofIt - dofNames_.begin(); //index for this degree of freedom in our member variables

            // Then open the outputDataFile
            string outputDataFilename = outputDir + "ma_" + dofNames_[dofIndex] + ".out";
            ofstream outputDataFile(outputDataFilename.c_str());
            if (!outputDataFile.is_open()) {
                cout << "ERROR: outputDataFile could not be open\n";
                exit(EXIT_FAILURE);
            }

            for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i)
                outputDataFile << dofMusclesIT->second.at(i) << "\t";
            outputDataFile << endl;

            vector <size_t> splineNums(dofMusclesIT->second.size());
            for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i) {
                vector<string>::iterator iter = find(muscleNames_.begin(), muscleNames_.end(), dofMusclesIT->second.at(i));
                if (iter == muscleNames_.end()) {
                    cout << "something went wrong\n";
                    exit(EXIT_FAILURE);
                }
                splineNums.at(i) = iter - muscleNames_.begin();
            }

            for (unsigned int j = 0; j < angleCombinations.size(); ++j) {
                for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i) {
                    outputDataFile << std::setprecision(NUMBER_DIGIT_OUTPUT) << std::fixed << -roundIt(splines_[splineNums[i]].getFirstDerivative(angleCombinations.at(j), dofIndex), DIGIT_NUM + 2) << "\t";

                }
                outputDataFile << endl;
            }

            outputDataFile.close();
        }

    }


    template <int N_DOF_SPLINESET>
    void SplineSet<N_DOF_SPLINESET>::evalMaDerivative(const string& outputDir, const  map<string, vector<string> >& musclesConnectedToDofs, const vector< vector< double > >& angleCombinations  ) {


      const int DIGIT_NUM = 8;
      const int NUMBER_DIGIT_OUTPUT = 8;



      // for all the degrees of freedom in the map
      for ( map<string, vector<string> >::const_iterator dofMusclesIT = musclesConnectedToDofs.begin(); dofMusclesIT!= musclesConnectedToDofs.end() ; ++dofMusclesIT) {
        vector<string>::iterator dofIt=find(dofNames_.begin(), dofNames_.end(), dofMusclesIT->first);
        if ( dofIt==dofNames_.end() ) {
          cout << "ERROR: dofsNames differ\n";
          exit(EXIT_FAILURE);
        }
        size_t dofIndex=dofIt-dofNames_.begin(); //index for this degree of freedom in our member variables

        // Then open the outputDataFile
        string outputDataFilename = outputDir +  "Dma_" + dofNames_[dofIndex] + ".out";
        ofstream outputDataFile(outputDataFilename.c_str());
        if (!outputDataFile.is_open()) {
          cout << "ERROR: outputDataFile could not be open\n";
          exit(EXIT_FAILURE);
        }

        for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i)
          outputDataFile << dofMusclesIT->second.at(i) << "\t";
        outputDataFile << endl;


        double nextValue;
        vector <size_t> splineNums(dofMusclesIT->second.size());
        for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i) {
             vector<string>::iterator iter = find(muscleNames_.begin(), muscleNames_.end(), dofMusclesIT->second.at(i));
             if (iter == muscleNames_.end()) {
               cout << "something went wrong\n";
               exit(EXIT_FAILURE);
             }
            splineNums.at(i)= iter - muscleNames_.begin();
        }

        for (unsigned int j = 0; j < angleCombinations.size(); ++j) {
          for (unsigned int i = 0; i < dofMusclesIT->second.size(); ++i) {
            outputDataFile << std::setprecision(NUMBER_DIGIT_OUTPUT) << std::fixed << -roundIt(splines_[splineNums[i]].getSecondDerivative(angleCombinations.at(j),dofIndex), DIGIT_NUM+2) << "\t";

          }
          outputDataFile << endl;
        }

      outputDataFile.close();
      }

    }


    template <int N_DOF_SPLINESET>
    vector< string > SplineSet<N_DOF_SPLINESET>::getDofNames() {
      return dofNames_;
    }

    template class SplineSet<7>;
    template class SplineSet<6>;
    template class SplineSet<5>;
    template class SplineSet<4>;
    template class SplineSet<3>;
    template class SplineSet<2>;
}