#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <algorithm>
#include <string>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_set>
#include <typeinfo>
#include <sstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include </root/llvm/openCLFeatureExtract/json.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#include <llvm/IRReader/IRReader.h>
#include "llvm/IR/Metadata.h"
#include "llvm/Pass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>



using namespace std;
using namespace llvm;
using json = nlohmann::json;

const unordered_set<string> BIN_OPS = {"add", "fadd", "sub", "fsub", "mul", "fmul", "udiv", "sdif", "fdiv", "urem", "srem", "frem"};
const unordered_set<string> BITBIN_OPS = {"shl", "lshr", "ashr", "and", "or", "xor"};
const unordered_set<string> VEC_OPS = {"extractelement", "insertelement", "shufflevector"};
const unordered_set<string> AGG_OPS = {"extractvalue", "insertvalue"};

const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace = 1;
const unsigned globalAddressSpace = 2;
const string usage = "Usage:\n\t-h help\n\t-f <bitcode file (.bc format)>\n\t-o <output file>\n\t-v verbose\n";

class InputParser{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(string(argv[i]));
        }
        
        const string& getCmdOption(const string &option) const{
            vector<string>::const_iterator itr;
            itr =  find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const string empty = "";
            return empty;
        }
        
        bool cmdOptionExists(const string &option) const{
            return find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        vector <string> tokens;
};

class FeatureStats {
    
public:
    int bbcount = 0;
    int funcount = 0;
 
    int binOpsCount = 0;
    int bitbinOpsCount = 0;
    int vecOpsCount = 0;
    int aggOpsCount = 0;
    int loadOpsCount = 0;
    int storeOpsCount = 0;
    int otherOpsCount = 0;
    
    int globalMemAcc = 0;
    int localMemAcc = 0;
    int privateMemAcc = 0;
    vector <string> instructions;

    // Global operator<< overload:
    friend ostream &operator<<(ostream &os, const FeatureStats &stats)
    {
        os << "\n###############################\n";
        os << "#Functions: " << stats.funcount << "\n";
        os << "#Basic Blocks: " << stats.bbcount << "\n";

        os << "\n#Bin Ops: " << stats.binOpsCount << "\n";
        os << "#Bit Bin Ops: " << stats.bitbinOpsCount << "\n";
        os << "#Vec Ops: " << stats.vecOpsCount << "\n";
        os << "#Agg Ops: " << stats.aggOpsCount << "\n";
        os << "#Load Ops: " << stats.loadOpsCount << "\n";
        os << "#Store Ops: " << stats.storeOpsCount << "\n";
        os << "#Other Ops: " << stats.otherOpsCount << "\n";
        
        os << "\n#Global Mem Access: " << stats.globalMemAcc << "\n";
        os << "#Local Mem Access: " << stats.localMemAcc << "\n";
        os << "#Private Mem Access: " << stats.privateMemAcc << "\n";
        os << "-------------------------------------------\n";
        os << "#Total Ops: " << stats.getTotalOpsCount();
        copy(stats.instructions.begin(), stats.instructions.end(), std::ostream_iterator<string>(std::cout, " "));
        
        return os;
    }
    
    int getTotalOpsCount() const {
        return binOpsCount + bitbinOpsCount + 
                vecOpsCount + aggOpsCount + otherOpsCount +
                loadOpsCount + storeOpsCount;
    }
};

string evalInstruction(const Instruction &inst, FeatureStats &stats);
bool checkAddrSpace(const unsigned addrSpaceId, FeatureStats &stats);
void writeToCSV(const string &outFile, const FeatureStats &stats);
void calculateNgrams(FeatureStats &stats);
void writeToJson(const string funName, vector <string> instructions);
vector<string> split(string strToSplit, char delimeter);
string join(const vector<string> & v, const string & delimiter = ",");

bool verbose = false;

// string join(const vector<string> v, const string & delimiter = ",") {
//     string out;
//     if (auto i = v.begin(); e = v.end(); i != e) {
//         out += *i++;
//         for (; i != e; ++i) out.append(delimiter).append(*i);
//     }
//     return out;
// }

std::vector<std::string> split(std::string strToSplit, char delimeter)
{
    std::stringstream ss(strToSplit);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, delimeter))
    {
       splittedStrings.push_back(item);
    }
    return splittedStrings;
}

int main(int argc, char* argv[]) {
    InputParser input(argc, argv);
    
    if (input.cmdOptionExists("-h")) {
        cout << usage;
        exit(0);
    }
    if (input.cmdOptionExists("-v")) {
        verbose = true;
    }
    const string &fileName = input.getCmdOption("-f");
    if (fileName.empty()){
        cout << usage;
        exit(1);
    }
    const string &new_dir = input.getCmdOption("-o");
    if (new_dir.empty()){
        cout << usage;
        exit(1);
    }
    
    
    FeatureStats stats;
    map<string, vector<string>> functionsMap;


    LLVMContext context;
    ErrorOr<unique_ptr<MemoryBuffer>> fileBuffer = MemoryBuffer::getFile(fileName);
    // if (error_code ec = fileBuffer.getError())
	// 	cout << "ERROR loading bitcode file: " << fileName << " -- " << ec.message() << "\n";
	// else
	// 	cout << "Bitcode file loaded: " << fileName << "\n";  
    
    MemoryBufferRef memRef = (*fileBuffer)->getMemBufferRef();
    Expected<unique_ptr<Module>> bcModule = parseBitcodeFile(memRef, context);

    // Attempt to Parser LLVM IR File instead of LLVM bitcode file

    // SMDiagnostic Err;
    // Expected<unique_ptr<Module>> bcModule = parseIRFile(argv[1], Err, context);
    // if (!bcModule) {
    //     cout << "??" << "\n";
    //     Err.print(argv[0], errs());
    // return 1;
    // } 
    // for (Module:: functions) {
    // }

    // owner, repository extraction from filename
    cout << fileName << "\n"; 
    vector <string> directories = split(fileName, '_');
    directories.erase(directories.end() - 1);
    string repo_name = directories.end()[-1];
    string owner_name = split(directories.end()[-2], '/').end()[-1];

    ostringstream oss;
    oss << new_dir << "/" << owner_name << "/" << repo_name;
    string path = oss.str();

    cout << path << "\n";
    boost::filesystem::create_directories(path);
    json functionsJson;

    // Iteration over Module functions
    (*bcModule) -> getFunctionList().begin();
    for (Module::const_iterator
        curFref = (*bcModule) -> getFunctionList().begin(),
        endFref = (*bcModule) -> getFunctionList().end();
        curFref != endFref; curFref++) {
        stats.funcount++;
        
        StringRef fun_name = (curFref) -> getName();
        vector <string> function_instructions;
        for (Function::const_iterator curBref = curFref->begin(), endBref = curFref->end();
         curBref != endBref; curBref++) {
             
            // Json::Value jsonArray;
            stats.bbcount++;
            for (BasicBlock::const_iterator curIref = curBref->begin(), endIref = curBref->end();
                 curIref != endIref; curIref++) {
                    //outs() << "Found an instruction: " << *curIref << "\n";        
                    string op = evalInstruction(*curIref, stats);
                    function_instructions.push_back(op);
                   // jsonArray.append(op);
            }
            // writeToJson(fun_name.str(), function_instructions);
        }
        if (fun_name.str().compare("") != 0 && fun_name.str().find("kfun") != string::npos) {
            functionsJson[fun_name.str()] = function_instructions;
        }
    }
    string functionFile = path + "/functions.json";
    cout << functionFile << "\n";
    std::ofstream output(functionFile);
    output << std::setw(2) << functionsJson << std::endl;
}


string evalInstruction(const Instruction &inst, FeatureStats &stats) {
    
    string opName = inst.getOpcodeName();
    stats.instructions.push_back(opName);
    if(BIN_OPS.find(opName) != BIN_OPS.end()) {
        stats.binOpsCount++;
    }
    else if(BITBIN_OPS.find(opName) != BITBIN_OPS.end()) {
        stats.bitbinOpsCount++;
    }
    else if(AGG_OPS.find(opName) != AGG_OPS.end()) {
        stats.aggOpsCount++;
    }
    else if(VEC_OPS.find(opName) != VEC_OPS.end()) {
        stats.vecOpsCount++;
    }
    else if(const LoadInst *li = dyn_cast<LoadInst>(&inst)) {
        stats.loadOpsCount++;
    }
    /*    
        //if (!checkAddrSpace(li->getPointerAddressSpace(), stats))
         //   li->dump();
    } else if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        stats.storeOpsCount++;
        //if (!checkAddrSpace(si->getPointerAddressSpace(), stats))
         //   li->dump();
    }*/ 
    else if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        stats.storeOpsCount++;
    }
    else {
        stats.otherOpsCount++;
    }
    return opName;
}

bool checkAddrSpace(const unsigned addrSpaceId, FeatureStats &stats) {
    if(addrSpaceId == localAddressSpace) {
        stats.localMemAcc++;
        return true;
    } else if(addrSpaceId == globalAddressSpace) {
        stats.globalMemAcc++;
        return true;
    } else if(addrSpaceId == privateAddressSpace) {
        stats.privateMemAcc++;
        return true;
    } else {
        cout << "WARNING: unhandled address space id: " << addrSpaceId << "\n";
        return false;
    }
}

void writeToCSV(const string &outFile, const FeatureStats &stats) {
    cout << "Writing to file: " << outFile << endl;
    
    ofstream out;
    out.open (outFile);
    out << "functions,bbs,binOps,bitBinOps,vecOps,aggOps,loadOps,storeOps,otherOps,totalOps,gMemAcc,lMemAcc,pMemAcc" << endl;
    out << stats.funcount << "," <<
            stats.bbcount << "," <<
            stats.binOpsCount << "," <<
            stats.bitbinOpsCount << "," <<
            stats.vecOpsCount << "," <<
            stats.aggOpsCount << "," <<
            stats.loadOpsCount << "," <<
            stats.storeOpsCount << "," <<
            stats.otherOpsCount << "," <<
            stats.getTotalOpsCount() << "," <<
            stats.globalMemAcc << "," <<
            stats.localMemAcc << "," <<
            stats.privateMemAcc << endl;
    out.close();
}
