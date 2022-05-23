// ***********************************************************************************
// Idefix MHD astrophysical code
// Copyright(C) 2020-2022 Geoffroy R. J. Lesur <geoffroy.lesur@univ-grenoble-alpes.fr>
// and other code contributors
// Licensed under CeCILL 2.1 License, see COPYING for more information
// ***********************************************************************************

#ifndef INPUT_HPP_
#define INPUT_HPP_

#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include "idefix.hpp"

using IdefixParamContainer = std::vector<std::string>;
using IdefixBlockContainer = std::map<std::string,IdefixParamContainer>;
using IdefixInputContainer = std::map<std::string,IdefixBlockContainer>;

class Input {
 public:
  // Constructor from a file
  Input (int, char ** );
  void ShowConfig();

  // Accessor to input parameters
  // the parameters are always: BlockName, EntryName, ParameterNumber (starting from 0)
  // These specialised functions are deprecated. Use the template Get<T> function.
  std::string GetString(std::string, std::string, int); ///< Read a string from the input file
  real GetReal(std::string, std::string, int);          ///< Read a real number from the input file
  int GetInt(std::string, std::string, int);            ///< Read an integer from the input file


  int CheckEntry(std::string, std::string);             ///< Check that a block+entry is present
                                                        ///< in the input file
  template<typename T>
  T Get(std::string, std::string, int);                 ///< read a variable from the input file
                                                        ///< (abort if not found)

  template<typename T>
  T GetOrSet(std::string, std::string, int, T);         ///<  read a variable from the input file
                                                        ///< (set it to T if not found)


  bool CheckBlock(std::string);                         ///< check that whether a block is defined
                                                        ///< in the input file
  bool CheckForAbort();                                 // have we been asked for an abort?
  void CheckForStopFile();                              // have we been asked for an abort from
                                                        // a stop file?

  Input();
  void PrintLogo();

  bool restartRequested{false};       //< Should we restart?
  int  restartFileNumber;             //< if yes, from which file?

  static bool abortRequested;         //< Did we receive an abort signal (USR2) from the system?

  bool tuningRequested{false};        //< whether the user has asked for loop-tuning

  int maxCycles{-1};                   //< whether we should perform a maximum number of cycles

  bool forceNoWrite{false};           //< explicitely disable all writes to disk

 private:
  std::string inputFileName;
  IdefixInputContainer  inputParameters;
  void ParseCommandLine(int , char **argv);
  static void signalHandler(int);
  std::vector<std::string> getDirectoryFiles();
  std::string getFileExtension(const std::string file_name);
  Kokkos::Timer timer;

  double lastStopFileCheck;
};

// Template functions

template<typename T>
T Input::Get(std::string blockName, std::string paramName, int num) {
  if(CheckEntry(blockName, paramName) <= num) {
      std::stringstream msg;
      msg << "Mandatory parameter [" << blockName << "]:" << paramName << "(" << num
          << "). Cannot be found in the input file" << std::endl;
      IDEFIX_ERROR(msg);
  }

  // Fetch it
  std::string paramString = inputParameters[blockName][paramName][num];
  T value;
  try {
    // The following mess with pointers is required since we do not have access to constexpr if
    // in c++ 14, hence we need to cast T to all of the available type we support.
    if(typeid(T) == typeid(int)) {
      int *v = reinterpret_cast<int*>( &value);
      double dv = std::stod(paramString, NULL);
      int iv = static_cast<int>(std::round(dv));
      if (std::abs((dv - iv)/dv) > 1e-14) {
        IDEFIX_WARNING("Detected a truncation error while reading an integer");
      }
      *v  = iv;
    } else if(typeid(T) == typeid(double)) {
      double *v = reinterpret_cast<double*>( &value);
      *v = std::stod(paramString, NULL);
    } else if(typeid(T) == typeid(float)) {
      float *v = reinterpret_cast<float*>( &value);
      *v = std::stof(paramString, NULL);
    } else if(typeid(T) == typeid(int64_t)) {
      int64_t *v = reinterpret_cast<int64_t *>( &value);
      *v = static_cast<int64_t>(std::round(std::stod(paramString, NULL)));
    } else if(typeid(T) == typeid(std::string)) {
      std::string *v = reinterpret_cast<std::string*>( &value);
      *v = paramString;
    } else if(typeid(T) == typeid(bool)) {
      bool *v = reinterpret_cast<bool*>( &value);
      // convert string to lower case
      std::for_each(paramString.begin(), paramString.end(), [](char & c){
        c = ::tolower(c);
      });
      if(paramString.compare("yes") == 0) {
        *v = true;
      } else if(paramString.compare("true") == 0) {
        *v = true;
      } else if(paramString.compare("1") == 0) {
        *v = true;
      } else if(paramString.compare("no") == 0) {
        *v = false;
      } else if(paramString.compare("false") == 0) {
        *v = false;
      } else if(paramString.compare("0") == 0) {
        *v = false;
      } else {
        std::stringstream msg;
        msg << "Boolean parameter [" << blockName << "]:" << paramName << "(" << num
          << ") cannot be interpreted as boolean in the input file." << std::endl
          << std::endl << "I read \"" << paramString << "\"" << std::endl
          << "Use \"yes\", \"true\" or \"1\" for boolean true ;"
          << " or \"no\", \"false\" or \"0\" for boolean false.";
        IDEFIX_ERROR(msg);
      }
    } else {
      IDEFIX_ERROR("Unknown type has been requested from the input file");
    }
  } catch(const std::exception& e) {
    std::stringstream errmsg;
    errmsg << e.what() << std::endl
          << "Input::Get: Error while reading [" << blockName << "]:" << paramName << "(" << num
          << ")." << std::endl
          << "\"" << paramString << "\" cannot be interpreted as required." << std::endl;
    IDEFIX_ERROR(errmsg);
  }
  return(value);
}

template<typename T>
T Input::GetOrSet(std::string blockName, std::string paramName, int num, T def) {
  int entrySize = CheckEntry(blockName, paramName);
  if(entrySize <= num ) {
    // the requested entry has not been set, add it (if we can), otherwise throw an error
    if(entrySize < 0 && num>0) {
      std::stringstream msg;
      msg << "Entry [" << blockName << "]:" << paramName << "is not defined." << std::endl
          << "Only the first (index 0) parameter can be set by default." << std::endl;
      IDEFIX_ERROR(msg);
    }
    if(entrySize+1 < num) {
      std::stringstream msg;
      msg << "Entry [" << blockName << "]:" << paramName << "has " << entrySize << " parameters."
          << std::endl << ". Only the " << entrySize  << "th (index " << entrySize-1
          << ") parameter can be set by default." << std::endl;
      IDEFIX_ERROR(msg);
    }
    std::stringstream strm;
    strm << std::boolalpha << def;
    inputParameters[blockName][paramName].push_back(strm.str());
  }
  return(Get<T>(blockName, paramName, num));
}


#endif // INPUT_HPP_
