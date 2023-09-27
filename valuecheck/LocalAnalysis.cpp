#include "llvm/Pass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include "llvm/Support/FileSystem.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IntrinsicInst.h"
#include <iostream>
#include <cxxabi.h>
#include "llvm/IR/Operator.h"
#include "llvm/Support/CommandLine.h"
#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
//#include "Util/PAM.h"


using namespace llvm;
using namespace std;
using namespace SVF;

#define DEBUG_TYPE "Liveness"
#define STDERR_FILENO 2

cl::opt<string> target("t", cl::desc("Specify target: all/fname"), cl::value_desc("target"));


//counter
int pointer_alias_counter = 0;
int value_alias_counter = 0;
int cursor_counter = 0;
map<string, int> FuncTotal;
map<string, map<int, int>> FuncUnused;
int pointer_count = 0;
bool SVF_integration = true;

//int plus_one_variable = 0;

bool unused_flag = true;

std::map<Function *, bool> DefCallUse;

inline std::string demangle(const char* name) 
{
    int status = -1; 
    std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free  };
    return (status == 0) ? res.get() : std::string(name);

}


namespace
{
struct LocalLiveness : public ModulePass
{
    string func_name = "test";
    static char ID;
    map<string, set<pair<Value*, const Instruction*>> > sum_unused_map;
    map<string, map<const Instruction*, set<const Instruction*>>> def_unused_map;
    map<string, map<Value*, DILocalVariable*>> localv_map;
    map<string, map<Value*, string>> du_map;
    //const TargetLibraryInfo TLI;
    LocalLiveness() : ModulePass(ID) {
    }

    void initialize(vector<const BasicBlock *> &BBList, string f_name, vector<set<Value *>> &UEVar, vector<set<Value *>> &VarKill, bool print_flag)
    {
        for (int i = 0; i < BBList.size(); ++i)
        {
            for (auto &inst : *BBList[i])
            {
                if (inst.getOpcode() == Instruction::Load)
                {
                    Value *v = inst.getOperand(0)->stripPointerCastsAndAliases();
                    if (isa<AllocaInst>(v))  
                    {
                        if(VarKill[i].find(v) == VarKill[i].end())
                        {
                            UEVar[i].insert(v);
                        }
                    }
                }
                if (inst.getOpcode() == Instruction::Store)
                {
                    Value *v = inst.getOperand(1)->stripPointerCastsAndAliases();
                    if(isa<AllocaInst>(v))
                    {
                        if(VarKill[i].find(v) == VarKill[i].end())
                            VarKill[i].insert(v);
                    }
                }
                if (auto call_inst = dyn_cast<CallBase>(&inst))
                {
                    Function *callee = call_inst->getCalledFunction();
                    if(callee == NULL)
                        continue;
                    string callee_name = callee->getName().str();
                    if(callee_name.compare("llvm.dbg.declare") == 0)
                    {
                        // declare local variable
                        auto dbgI = dyn_cast<DbgDeclareInst>(call_inst);
                        Value *map_k = dbgI->getAddress();//cast<MDNode>(call_inst->getOperand(0));
                        DILocalVariable *locv =  dbgI->getVariable();
                        if(locv == NULL)
                            continue;
                        //string v_name = locv->getName();
                        localv_map[f_name][map_k] = locv;
                    }
                }
            }
        }
    }

    void initializeDefOut(int bbIndex, map<const BasicBlock *, int> &BBMap, vector<const BasicBlock *> &BBList, vector<map<Value*, set<const Instruction *>>> &DefIn, vector<map<Value*, set<const Instruction *>>> &DefOut)
    {
        const BasicBlock *bb = BBList[bbIndex];
        for (const BasicBlock *Succ : successors(bb))
        {
            int i = BBMap.at(Succ);
            // Merge DefIn[i] into DefOut[bbIndex]
            for(auto it = DefIn[i].begin(); it != DefIn[i].end(); ++it)
            {
                Value *v = it->first;
                set<const Instruction *> &defout = DefOut[bbIndex][v];
                set<const Instruction *> &defin = it->second;
                set_union(defout.begin(), defout.end(), 
                    defin.begin(), defin.end(), 
                    inserter(defout, defout.begin()));
            }
        }
    }

    void calDefIn(vector<map<Value*, set<const Instruction *>>> &DefIn, vector<map<Value*, set<const Instruction *>>> &DefOut, const BasicBlock* bb, int bbIndex)
    {
        DefIn[bbIndex] = DefOut[bbIndex];
        // traverse each I in BB, compute DefIn
        for (auto &inst : *bb)
        {
            if (inst.getOpcode() == Instruction::Store)
            {
                Value *v = inst.getOperand(1)->stripPointerCastsAndAliases();
                if(isa<AllocaInst>(v))
                {
                    DefIn[bbIndex][v].clear();
                    DefIn[bbIndex][v].insert(&inst);
                }
            }
        }
    }

    bool computeLiveOut(int bbIndex, map<const BasicBlock *, int> &BBMap, vector<const BasicBlock *> &BBList, vector<set<Value *>> &UEVar, vector<set<Value *>> &VarKill, vector<set<Value *>> &LiveIn, vector<set<Value *>> &LiveOut, bool print_flag, vector<map<Value*, set<const Instruction *>>> &DefIn, vector<map<Value*, set<const Instruction *>>> &DefOut)
    {
        print_flag = false;
        const BasicBlock *bb = BBList[bbIndex];
        // get Liveout[i]
        for (const BasicBlock *Succ : successors(bb))
        {
            int i = BBMap.at(Succ);
            set_union(LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), 
                    LiveIn[i].begin(), LiveIn[i].end(), 
                    inserter(LiveOut[bbIndex], LiveOut[bbIndex].begin()));
            // union the def set of all successors
            
        }

        // Compute LiveOut(x) - VarKill(x) and store in LiveIn
        set_difference(LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), 
                VarKill[bbIndex].begin(), VarKill[bbIndex].end(), 
                inserter(LiveIn[bbIndex], LiveIn[bbIndex].begin()));
        // LiveIn = LiveIn union UEVar[bbIndex]
        set_union(LiveIn[bbIndex].begin(), LiveIn[bbIndex].end(),
                UEVar[bbIndex].begin(), UEVar[bbIndex].end(),
                inserter(LiveIn[bbIndex], LiveIn[bbIndex].begin()));

        // Add the defined variables to DefSet
        calDefIn(DefIn, DefOut, bb, bbIndex);
        //Compute Defout
        initializeDefOut(bbIndex, BBMap, BBList, DefIn, DefOut);

        return true;
    }
 
    void computeDef(set<pair<Value*, const Instruction*>> &UnusedDef, map<const BasicBlock*, int> &BBMap, vector<const BasicBlock *> &BBList,vector<set<Value *>> &VarKill, vector<set<Value *>>&LiveOut, string curr_name, bool print_flag, vector<map<Value*, set<const Instruction *>>> &DefOut, map<const Instruction*, set<const Instruction *>> &OverwriteDef)
    {
        // Get unuseddef 
        for(auto &BB : BBList)
        {
            int idx = BBMap[BB];
            // go through the bb reversely and find each def inst 
            // if it's not in liveout
            // record it into UnusedDef
            set<Value*> curr_liveout(LiveOut[idx].begin(), LiveOut[idx].end());
            map<Value*, set<const Instruction*>> curr_def(DefOut[idx].begin(), DefOut[idx].end());
            for(auto I = BB->rbegin(), EI = BB->rend(); I!= EI; I++)
            {
                // check whether generate a def (and the def is unused in callee)
                switch(I->getOpcode())
                {
                    case Instruction::Store:
                    {
                        // if a def
                        Value *v = I->getOperand(1)->stripPointerCastsAndAliases();
                        if(isa<AllocaInst>(v))
                        {
                            // check curr_liveout
                            // for(auto &liveout_v: curr_liveout)
                            // {
                            // }
                            if(curr_liveout.find(v) == curr_liveout.end())
                            {
                                UnusedDef.insert(make_pair(v, &(*I)));  
                                // Check last def
                                if(curr_def.find(v) != curr_def.end())
                                {
                                    OverwriteDef[&(*I)] = curr_def[v];
                                }
                            }
                            auto it = curr_liveout.find(v);
                            if(it!= curr_liveout.end())
                            {
                                curr_liveout.erase(it);
                            }
                            // add to curr_def
                            curr_def[v].clear();
                            curr_def[v].insert(&(*I));
                        }
                        break;
                    }
                    case Instruction::Load:
                    {
                        Value *v = I->getOperand(0)->stripPointerCastsAndAliases();
                        if(isa<AllocaInst>(v))
                        {
                            // add to liveness
                            curr_liveout.insert(v);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    bool countPts(const Function *F, PointerAnalysis* pta, const Value* val, ICFG* icfg)
    {
        pointer_count += 1;
        if(pointer_count == 1)
            return false;
        if(val->getType()->isPointerTy() == false)
        {
            return val->hasOneUse() == false;
        }
        std::string str;
        raw_string_ostream rawstr(str);

        SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);

        NodeID pNodeId = pta->getPAG()->getValueNode(svfval);
        //const PointsTo& ns = pta->getPts(pNodeId);
        const NodeSet& ns = pta->getRevPts(pNodeId+1);
        return ns.size() - 1 > 0; 
    }

    bool traverseFunc(const Function *F, bool print_flag, PointerAnalysis* ander, ICFG* icfg)
    {
        string f_name = F->getName().str();
        //errs() << "Function name:" << f_name << "\n";
        
        if(f_name.find("llvm.") != string::npos)
            return false;
        // go through the func and check global variable use
        vector<const BasicBlock *> BBList;  //vector to store all the basic blocks
        map<const BasicBlock *, int> BBMap; //map to store the basic block and it's index
        int count = 0;
        // for (auto bb = F->begin(); bb != F->end(); ++bb)
        //     for (auto i = bb->begin(); i != bb->end(); ++i)
        //         outs() << "253:" << &(*i) << "\n";
        for (auto bb = F->begin(); bb != F->end(); ++bb)
        {
            const BasicBlock &basic_block = *bb;
            BBList.push_back(&basic_block);
            BBMap[&basic_block] = count;
            count++;
        }
        
        if(count == 0)
        {
            return false;
        }
        // initializing the UEVar,VarKill,LiveOut vectors
        // Cache UEVar and VarKill
        auto uk_map = make_pair(vector<set<Value*>>(count), vector<set<Value*>>(count));
        // initialize: print out use/def/call 
        initialize(BBList, f_name, uk_map.first, uk_map.second, print_flag);

        vector<set<Value *>> LiveIn(count);
        vector<set<Value *>> LiveOut(count);
        vector<map<Value*, set<const Instruction *>>> DefIn(count);
        vector<map<Value*, set<const Instruction *>>> DefOut(count);

        // iterative algorithm to compute LiveOut
        bool flag = true;
        int bblist_size = BBList.size();
        // compute livein/liveout based on uvar and vkill
        while (flag)
        {
            flag = false;
            for (int i = 0; i < bblist_size; ++i)
            {
                set<Value *> prev = LiveIn[bblist_size-i-1];
                computeLiveOut(bblist_size - i - 1, BBMap, BBList, uk_map.first, uk_map.second, LiveIn, LiveOut, print_flag, DefIn, DefOut);
                if (
                        !std::equal(
                            prev.begin(), prev.end(), 
                            LiveIn[bblist_size- i-1].begin(), LiveIn[bblist_size - i - 1].end())
                    )
                    flag = true;
            }
        }
        // compute external use based on livein[0]
        set<pair<Value*, const Instruction*>> UnusedDef;
        map<const Instruction*, set<const Instruction *>> OverwriteDef; // <overwritten inst, last def inst>
        // compute unuseddef/def
        computeDef(UnusedDef, BBMap, BBList, uk_map.second, LiveOut, f_name, print_flag, DefOut, OverwriteDef);
        
        set<Value*> point_to_set;
        for(auto &v: UnusedDef)
        {
            //errs() << "UnusedDef:" << *v.first << "\n";
            if(countPts(F, ander, v.first, icfg))
            {
                DILocalVariable *lv = localv_map[f_name][v.first];
                if(lv != NULL)
                {
                    fprintf(stderr, "[Pointer Alias]%s:\n@%s", f_name.c_str(), lv->getName().data());
                    const DIFile *fname = lv->getFile();
                    const DebugLoc &loc = v.second->getDebugLoc();
                    if(fname)
                        fprintf(stderr, "{%s/%s:%d}\n", fname->getDirectory().data(), fname->getFilename().data(), loc? loc->getLine():lv->getLine());
                    else
                        fprintf(stderr, "{null/null:-1}\n");
                }
                else
                {
                    fprintf(stderr, "[PointerAlias]%s:\n@%s{:}\n", f_name.c_str(), lv==NULL? "NULL":lv->getName().data());
                }
                pointer_alias_counter += 1;
                continue;
            }
            sum_unused_map[f_name].insert(v);
            def_unused_map[f_name][v.second] = OverwriteDef[v.second];
        }
        return true;
    }

    bool checkZeroDef(Instruction *I)
    {
        if(!isa<StoreInst>(I))
            errs() << "382: Not a store inst?\n";
        if(Constant *c = dyn_cast<Constant>(dyn_cast<StoreInst>(I)->getValueOperand()))
        {
            c = c->stripPointerCasts();
            if(c->isNullValue() || c->isZeroValue())
                return true;
        }
        return false;
    }
 
    bool checkRetValue(Instruction *I)
    {
        if(!isa<StoreInst>(I))
            errs() << "382: Not a store inst?\n";
        if(CallBase *c = dyn_cast<CallBase>(cast<StoreInst>(I)->getValueOperand()))
            return true;
        return false;
    }

    bool checkCursor(const Function *f, const Value *v, const SVFG* vfg, SVFIR* pag)
    {
        //if(!isa<StoreInst>(I))
        //    return false;
        SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(v);
        PAGNode* pNode = pag->getGNode(pag->getValueNode(svfval));
        //v->print(errs());fprintf(stderr, "\n");
        
        if(pNode == NULL)
            fprintf(stderr, "pNode is NULL\n");
        const VFGNode* vNode = vfg->getDefSVFGNode(pNode);
        int cursor_count = 0;
        int total_count = 0;
        bool cursor_flag = false;
        for (VFGNode::const_iterator it = vNode->OutEdgeBegin(), eit =
                    vNode->OutEdgeEnd(); it != eit; ++it)
        {
            VFGEdge* edge = *it;
            VFGNode* succNode = edge->getDstNode();
            if(!succNode)
                continue;
            const SVFValue* succVal = succNode->getValue();
            const Value *succ_v = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(succVal);
            if(isa<Instruction>(succ_v))
            {
                const Instruction *succ_v_I = cast<Instruction>(succ_v);
                //succ_v_I->print(errs());fprintf(stderr, "\n");
                if(succ_v_I->getOpcode() == Instruction::Add)
                {
                    const Instruction *useI = cast<Instruction>(v);
                    Value *op0 = useI->getOperand(0);
                    Instruction *op0_I = dyn_cast<Instruction>(op0);
                    // if the value is get plused by 1 
                    if(isa<Constant>(op0_I->getOperand(1)))
                    {
                        cursor_count += 1;
                        //errs() << "431:" << cursor_count << "\n";
                    }
                }
                total_count += 1;
                //errs() << "436:" << total_count << "\n";
            }
        }
        if(cursor_count/total_count > 0.7)
            cursor_flag = true;
        return cursor_flag;
    }
   
    bool checkArray(const Instruction *I, const Value* v)
    {
        Type *t = v->getType();
        // check whether it is ArrayType
        if(t->isArrayTy())
        {
            //errs() << "checkArray: " << *I << "\n";
            return true;
        }
    }
    
    bool checkValueAlias(const Instruction *I, const Value* v, const SVFG* vfg)
    {
        // go through inst following I to whether see the alias are used
        // if has more than one use return true
        // else return false
        if(isa<GlobalVariable>(v) && ! v->hasOneUse())
        {
            return true;
        }
        if(SVF_integration)
        {
            // print v
            //errs() << "392:";I->print(errs());errs() << "\n";
            SVFIR* pag = SVFIR::getPAG();
            SVFValue* svfval = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(I);
            PAGNode* pNode = pag->getGNode(pag->getValueNode(svfval));
            //print v
            NodeID vID = pag->getValueNode(svfval);
            if(vfg->hasSVFGNode(vID))
            {
                const VFGNode* vNode = vfg->getSVFGNode(vID);
                if(vNode->getOutEdges().size() > 1)// || isa<Constant>(v))
                {
                    return true;
                }
            }
        }
        else
        {
            if(v->hasOneUse())
                return false;
        }
        return false;
    }

    // bool checkInlineVariable(Function *f, Value *v)
    // {
        
    //     // get meta data of the variable
    //     auto *subp = localv_map[f->getName().str()][v]->getScope()->getFile();
    //     if(subp == NULL)
    //     {
    //         return true;
    //     }
    //     string v_file = subp->getFilename().str();
    //     auto *fmeta = f->getSubprogram()->getFile();
    //     string f_file = fmeta->getFile()->getFilename().str();
    //     if(v_file.c_str()[0] == '/')//fmeta != subp)
    //     {
    //         return true;
    //     }
    //     else return false;
    // }

    // bool checkIgnoreParameter(Instruction *I, Value *v)
    // {
    //     Value *src = I->getOperand(0);
    //     Value *des = I->getOperand(1);
    //     if(isa<GetElementPtrInst>(des))
    //         return false;// field of struct

    //     Function *f = I->getFunction();
    //     bool flag = false;
    //     for(auto it = f->arg_begin(), ite = f->arg_end(); it!=ite; it++)
    //     {
    //         if(dyn_cast<Value>(&(*it)) == src)
    //         {
    //             flag =true;
    //             break;
    //         }                
    //     }
    //     if(flag && ! v->hasOneUse())
    //     {
    //         // is a parameter assigned value
    //         // check whether it's covered
    //         return true;
    //     }
    //     return false;
    // }
 
    bool checkParameter(Instruction *I, Value *v, int &param)
    {
        Value *src = I->getOperand(0);
        Value *des = I->getOperand(1);
        if(isa<GetElementPtrInst>(des))
            return false;// field of struct

        Function *f = I->getFunction();
        std::string str;
        llvm::raw_string_ostream rso(str);
        f->getFunctionType()->print(rso);
        
        bool flag = false;
        int idx = 0;
        for(auto it = f->arg_begin(), ite = f->arg_end(); it!=ite; it++)
        {
            if(dyn_cast<Value>(&(*it)) == src)
            {
                flag =true;
                param = idx;
                if (FuncUnused.find(str) == FuncUnused.end())
                    FuncUnused[str] = {};
                if (FuncUnused[str].find(param) == FuncUnused[str].end())
                    FuncUnused[str][param] = 0;
                FuncUnused[str][param] += 1;
                errs() << "475:" << str << " " << FuncUnused[str][param] << "\n";
                break;
            }       
            idx++;         
        }
        return flag && f->hasNUsesOrMore(1);
    }
   
    bool runOnModule(Module &M) override
    {
        bool all = !target.compare("all");
        //first Pass, do not get into any func traverse
        vector<string> moduleVec;
        moduleVec.push_back(M.getName().str());
        fprintf(stderr, "%s\n", M.getName().str().c_str());
        SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleVec);
        if(svfModule == NULL)
        {
            return false;
        }
        /// Build Program Assignment Graph (SVFIR)
        SVFIRBuilder builder(svfModule);
        SVFIR* pag = builder.build();
        
        ICFG* icfg = pag->getICFG();
        /// Create Andersen's pointer analysis
        Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
        //return 0;
        ander->disablePrintStat();
        SVFG* svfg = NULL;
        SVFGBuilder svfBuilder;
        svfg = svfBuilder.buildFullSVFG(ander);
    
        for(auto it = svfModule->begin(); it != svfModule->end(); ++it)
        {
            const Function *F = SVF::LLVMUtil::getLLVMFunction(LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(*it));
            std::string str;
            llvm::raw_string_ostream rso(str);
            F->getFunctionType()->print(rso);
            if (FuncTotal.find(str) == FuncTotal.end())
                FuncTotal[str] = 0;
            FuncTotal[str] = FuncTotal[str] + 1;
            if(!all)
            {
                string orig_fname = demangle(F->getName().str().c_str());
                if(target.compare(orig_fname) == 0)
                {
                    traverseFunc(F, true, ander, icfg);
                }
            }
            else{
                traverseFunc(F, true, ander, icfg);
            }
        }

        int count_alias = 0;
        for(auto &item : sum_unused_map)
        {
            if(item.second.empty())
                continue;
            string orig_fname = demangle(item.first.c_str());
            size_t found = orig_fname.find("(");
            if(found != string::npos)
                orig_fname = orig_fname.substr(0, found);
            for(auto &v : item.second)
            {
                const Function *func = v.second->getFunction();
                auto it = localv_map[item.first].find(v.first);
                if(it == localv_map[item.first].end())
                    continue;
                DILocalVariable *locv = it->second;
                if(locv == NULL)
                    continue;
                string vname = locv->getName().str();
                if(vname.empty() || vname.compare("_dbg_level") == 0)
                    continue;
                std::string str;
                llvm::raw_string_ostream rso(str);
                func->getFunctionType()->print(rso);
                //svfg->dump("test.dot");
                //
                //fprintf(stderr, "%s:\n", orig_fname.c_str());
                //fprintf(stderr,  "@%s", vname.c_str());
                //
                if(checkValueAlias(v.second, v.first, svfg))
                {
                    fprintf(stderr, "[ValueAlias]");
                    value_alias_counter += 1;
                }
                
                if(checkCursor(func, v.first, svfg, pag))
                {
                    fprintf(stderr, "[Cursor]");
                    cursor_counter += 1;
                }
                if(checkArray(v.second, v.first))
                {
                    continue;
                }
                fprintf(stderr, "%s:\n", orig_fname.c_str());
                fprintf(stderr,  "@%s", vname.c_str());

                const DIFile *fname = locv->getFile();
                const DebugLoc &loc = v.second->getDebugLoc();
                set<const Instruction*> overwriteI(def_unused_map[item.first][v.second]);
                if(fname) {
                    fprintf(stderr, "{%s/%s:%d}", fname->getDirectory().data(), fname->getFilename().data(), loc? loc->getLine():locv->getLine());
                    for(auto &i : overwriteI)
                    {
                        const DebugLoc &overwriteLoc = i->getDebugLoc();
                        if(overwriteLoc)
                        {
                            fprintf(stderr, "|{%s/%s:%d}", fname->getDirectory().data(), fname->getFilename().data(), overwriteLoc->getLine());
                        }
                    }
                    
                    // If it's function call, get callee
                    Value *assigned = v.second->getOperand(0);
                    if(isa<CallInst>(assigned))
                    {
                        const CallInst *call = dyn_cast<CallInst>(assigned);
                        Function *callee = call->getCalledFunction();
                        if(callee)
                        {
                            // get callee definition location
                            const DISubprogram *sub = callee->getSubprogram();
                            if(sub)
                            {
                                fprintf(stderr, "|{%s/%s:%d}", sub->getDirectory().data(), sub->getFilename().data(), sub->getLine());
                            }
                        }
                    }

                    // If it's argument, get caller
                    if(locv->getArg())
                    {
                        // get all callers of Function* func
                        for(auto &u : func->uses())
                        {
                            const User *user = u.getUser();
                            if(isa<CallInst>(user))
                            {
                                const CallInst *call = dyn_cast<CallInst>(user);
                                //print debug loc of call
                                const DebugLoc &callLoc = call->getDebugLoc();
                                if(callLoc)
                                {
                                    fprintf(stderr, "|{%s/%s:%d}", fname->getDirectory().data(), fname->getFilename().data(), callLoc->getLine());
                                }
                            }
                        }
                    }
                }
                fprintf(stderr, "\n");
            }
        }
        return false;
    }
}; // end of struct Liveness
} // end of anonymous namespace

char LocalLiveness::ID = 0;
static RegisterPass<LocalLiveness> X("LocalLiveness", "Local Liveness Analysis",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);
