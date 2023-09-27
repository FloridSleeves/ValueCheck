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

using namespace llvm;
using namespace std;
#define DEBUG_TYPE "Liveness"
#define STDERR_FILENO 2

cl::opt<string> target("t", cl::desc("Specify target: all/fname"), cl::value_desc("target"));


//counter
int pointer_alias_counter = 0;
int value_alias_counter = 0;
int resource_cleanup_counter = 0;
int inline_variable_counter = 0;
//int plus_one_variable = 0;

bool unused_flag = true;

std::map<Function *, pair<int, int>> DefCallUse; // <func, <total, used>>

inline std::string demangle(const char* name) 
{
    int status = -1; 
    std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free  };
    return (status == 0) ? res.get() : std::string(name);

}


namespace
{
struct TransientLiveness : public ModulePass
{
    //string func_name = "test";
    static char ID;
    map<string, set<pair<Value*, Instruction*>> > sum_unused_map;
    map<string, map<Value*, DILocalVariable*>> localv_map;
    map<string, map<Value*, string>> du_map;
    //const TargetLibraryInfo TLI;
    TransientLiveness() : ModulePass(ID) {
    }


    void initialize(vector<BasicBlock *> &BBList, string f_name, vector<set<Value *>> &UEVar, vector<set<Value *>> &VarKill, bool print_flag)
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
                if (CallBase *call_inst = dyn_cast<CallBase>(&inst))
                {
                }
            }
        }
    }

    bool computeLiveOut(int bbIndex, map<BasicBlock *, int> &BBMap, vector<BasicBlock *> &BBList, vector<set<Value *>> &UEVar, vector<set<Value *>> &VarKill, vector<set<Value *>> &LiveIn, vector<set<Value *>> &LiveOut, bool print_flag)
    {
        print_flag = false;
        BasicBlock *bb = BBList[bbIndex];
        // get Liveout[i]
        for (BasicBlock *Succ : successors(bb))
        {
            int i = BBMap.at(Succ);
            set_union(LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), 
                    LiveIn[i].begin(), LiveIn[i].end(), 
                    inserter(LiveOut[bbIndex], LiveOut[bbIndex].begin()));
        }
        // Compute LiveOut(x) - VarKill(x) and store in LiveIn
        set_difference(LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), 
                VarKill[bbIndex].begin(), VarKill[bbIndex].end(), 
                inserter(LiveIn[bbIndex], LiveIn[bbIndex].begin()));

        // LiveIn = LiveIn union UEVar[bbIndex]
        set_union(LiveIn[bbIndex].begin(), LiveIn[bbIndex].end(),
                UEVar[bbIndex].begin(), UEVar[bbIndex].end(),
                inserter(LiveIn[bbIndex], LiveIn[bbIndex].begin()));
        return true;
    }
 
    void computeDef(set<pair<Value*, Instruction*>> &UnusedDef, set<Value*> &Def, map<BasicBlock*, int> &BBMap, vector<BasicBlock *> &BBList,vector<set<Value *>> &VarKill, vector<set<Value *>>&LiveOut, string curr_name, bool print_flag)
    {
        // Get unuseddef 
        for(auto &BB : BBList)
        {
            int idx = BBMap[BB];
            // go through the bb reversely and find each def inst 
            // if it's not in liveout
            // record it into UnusedDef
            set<Value*> curr_liveout(LiveOut[idx].begin(), LiveOut[idx].end());
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
                            for(auto &liveout_v: curr_liveout)
                            {
                            }
                            if(curr_liveout.find(v) == curr_liveout.end())
                            {
                                UnusedDef.insert(make_pair(v, &(*I)));                                
                            }
                            auto it = curr_liveout.find(v);
                            if(it!= curr_liveout.end())
                            {
                                curr_liveout.erase(it);
                            }
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

    void addPointTo(Function *F, set<Value*>&point_to_set)
    {
        string f_name = F->getName().str();
        if(f_name.find("llvm.") != string::npos)
            return;
        for (auto &B : *F)
        {
            for (auto &inst : B)
            {
                if (inst.getOpcode() == Instruction::Load)
                {
                }
                if (inst.getOpcode() == Instruction::Store)
                {
                    Value *v = inst.getOperand(0)->stripPointerCastsAndAliases();
                    if(isa<AllocaInst>(v))
                    {
                        point_to_set.insert(v);
                    }
                }
                if (inst.getOpcode() == Instruction::Call || inst.getOpcode() == Instruction::Invoke)
                {
                    CallBase *call_inst = cast<CallBase>(&inst);
                    for(auto arg_i = call_inst->arg_begin(), arg_e = call_inst->arg_end(); arg_i != arg_e; arg_i++)
                    {
                        Value *orig_v = arg_i->get()->stripPointerCastsAndAliases();
                        if(isa<AllocaInst>(orig_v))
                            point_to_set.insert(orig_v);
                    }
                }
                if (inst.getOpcode() == Instruction::Select)
                {
                    SelectInst *selectI = dyn_cast<SelectInst>(&inst);
                    Value *vlist[3];
                    vlist[0] = selectI->getCondition()->stripPointerCastsAndAliases();
                    vlist[1] = selectI->getTrueValue()->stripPointerCastsAndAliases();
                    vlist[2] = selectI->getFalseValue()->stripPointerCastsAndAliases();
                    for(int j = 0; j<3; j++)
                    {
                        if(isa<AllocaInst>(vlist[j]))
                        {
                            point_to_set.insert(vlist[j]);
                        }
                    }
                }
                if (inst.getOpcode() == Instruction::PHI)
                {
                    PHINode *phiI = dyn_cast<PHINode>(&inst);
                    int incomingNum = phiI->getNumIncomingValues();
                    for(int j = 0; j < incomingNum; j++)
                    {
                        Value *incomingV = phiI->getIncomingValue(j)->stripPointerCastsAndAliases();
                        if(isa<AllocaInst>(incomingV))
                        {
                            point_to_set.insert(incomingV);
                        }
                    }
                }
                if(inst.getOpcode() == Instruction::BitCast)
                {
                    BitCastInst *bcI = dyn_cast<BitCastInst>(&inst);
                    Value *src = bcI->getOperand(0);
                    if(isa<AllocaInst>(src))
                        point_to_set.insert(src);
                }
                if(inst.getOpcode() == Instruction::GetElementPtr)
                {
                    GetElementPtrInst *gepI = dyn_cast<GetElementPtrInst>(&inst);
                    Value *src = gepI->getOperand(0);
                    if(isa<AllocaInst>(src))
                        point_to_set.insert(src);
                }

            }
        }
        return;
    }

    bool traverseFunc(Function *F, bool print_flag)
    {
        set<Value*> Def, Use;
        string f_name = F->getName().str();
        
        
        if(f_name.find("llvm.") != string::npos)
            return false;
        // go through the func and check global variable use
        vector<BasicBlock *> BBList;  //vector to store all the basic blocks
        map<BasicBlock *, int> BBMap; //map to store the basic block and it's index
        int count = 0;
        for (auto &basic_block : *F)
        {
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
                computeLiveOut(bblist_size - i - 1, BBMap, BBList, uk_map.first, uk_map.second, LiveIn, LiveOut, print_flag);
                if (
                        !std::equal(
                            prev.begin(), prev.end(), 
                            LiveIn[bblist_size- i-1].begin(), LiveIn[bblist_size - i - 1].end())
                    )
                    flag = true;
            }
        }
        // compute external use based on livein[0]
        set<pair<Value*, Instruction*>> UnusedDef;
        // compute unuseddef/def
        computeDef(UnusedDef, Def, BBMap, BBList, uk_map.second, LiveOut, f_name, print_flag);
        set<Value*> point_to_set;
        addPointTo(F, point_to_set);
        for(auto &v: UnusedDef)
        {
            if(point_to_set.find(v.first) != point_to_set.end())
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
   
    bool checkValueAlias(Function *f, Instruction* I)
    {
        // go through inst following I to whether see the alias are used
        // if has more than one use return true
        // else return false
        if(!isa<StoreInst>(I))
            errs() << "382: Not a store inst?\n";
        Value *v = cast<StoreInst>(I)->getValueOperand()->stripPointerCastsAndAliases();
        
        if(isa<GlobalVariable>(v) && ! v->hasOneUse())
        {
            return true;
        }

        if(v->hasOneUse())// || isa<Constant>(v))
        {
            return false;
        }
        return true;
    }

    bool checkInlineVariable(Function *f, Value *v)
    {
        
        // get meta data of the variable
        auto *subp = localv_map[f->getName().str()][v]->getScope()->getFile();
        if(subp == NULL)
        {
            return true;
        }
        string v_file = subp->getFilename().str();
        auto *fmeta = f->getSubprogram()->getFile();
        string f_file = fmeta->getFile()->getFilename().str();
        if(v_file.c_str()[0] == '/')//fmeta != subp)
        {
            return true;
        }
        else return false;
    }

    bool checkIgnoreParameter(Instruction *I, Value *v)
    {
        Value *src = I->getOperand(0);
        Value *des = I->getOperand(1);
        if(isa<GetElementPtrInst>(des))
            return false;// field of struct

        Function *f = I->getFunction();
        bool flag = false;
        for(auto it = f->arg_begin(), ite = f->arg_end(); it!=ite; it++)
        {
            if(dyn_cast<Value>(&(*it)) == src)
            {
                flag =true;
                break;
            }                
        }
        if(flag && ! v->hasOneUse())
        {
            // is a parameter assigned value
            // check whether it's covered
            return true;
        }
        return false;
    }
 
    bool checkParameter(Instruction *I, Value *v)
    {
        Value *src = I->getOperand(0);
        Value *des = I->getOperand(1);
        if(isa<GetElementPtrInst>(des))
            return false;// field of struct

        Function *f = I->getFunction();
        bool flag = false;
        for(auto it = f->arg_begin(), ite = f->arg_end(); it!=ite; it++)
        {
            if(dyn_cast<Value>(&(*it)) == src)
            {
                flag =true;
                break;
            }                
        }
        return flag;
    }
   
    bool checkPlusOne(Instruction *I, Value *v)
    {
        if(!isa<StoreInst>(I))
            return false;

        Value *op0 = I->getOperand(0);
        if(GetElementPtrInst *op0_I = dyn_cast<GetElementPtrInst>(op0))
        {
            // if the value is get plused by 1 
            if(isa<Constant>(op0_I->getOperand(1)))
            {
                return true;
            }
        }
        return false;
    }

    void computeDUSig(Module &M)
    {

    }

    void detectTransientValue(Function &F, bool SecondPass) {
        for(auto &BB: F)
        {
            string func_name = F.getName().str();
            for(auto &I: BB) {
                if (!isa<CallBase>(I))
                    continue;
                // build local variable map
                if(!SecondPass)
                {
                    CallBase *call_inst = dyn_cast<CallBase>(&I);
                    Function *callee = call_inst->getCalledFunction();
                    if(callee == NULL)
                        continue;
                    string callee_name = callee->getName().str();
                    if(callee_name.compare("llvm.dbg.declare") == 0)
                    {
                        // declare local variable
                        DbgDeclareInst *dbgI = dyn_cast<DbgDeclareInst>(call_inst);
                        Value *map_k = dbgI->getAddress();
                        if(map_k)
                            map_k = map_k->stripPointerCastsAndAliases();//cast<MDNode>(call_inst->getOperand(0));
                        else
                            continue;
                        DILocalVariable *locv =  dbgI->getVariable();
                        if(locv == NULL)
                            continue;
                        //string v_name = locv->getName();
                        localv_map[func_name][map_k] = locv;
                    }
                    else if (callee_name.compare("llvm.dbg.value") == 0)
                    {
                        DbgValueInst *dbgI = dyn_cast<DbgValueInst>(call_inst);
                        Value *dbgv = dbgI->getValue()->stripPointerCastsAndAliases();
                        DILocalVariable *dbg_var = dbgI->getVariable();
                        if(dbg_var == NULL)
                            continue;
                        localv_map[func_name][dbgv] = dbg_var;
                    }
                }   
                
                CallBase *Call = dyn_cast<CallBase>(&I);
                if(!Call)
                    continue;
                Function *callee = Call->getCalledFunction();
                if(!callee) 
                    continue;
                StringRef fname = callee->getName();
                if(fname.find("llvm.dbg") != StringRef::npos)
                    continue;
                if(I.use_empty() && SecondPass)
                {
                    double usedRatio = (double)DefCallUse[callee].second/DefCallUse[callee].first;
                    
                    DebugLoc loc = I.getDebugLoc();
                    if(!loc)
                        continue;
                    if(fname.str() != "base64url_encode" &&  usedRatio < 0.5)
                        fprintf(stderr, "[PeerDef]");
                    fprintf(stderr, "%s:\n", F.getName().str().c_str());
                    fprintf(stderr,  "@*ret*%s", fname.str().c_str());
                    const DIFile *fname = loc->getFile();
                    if(fname) {
                        fprintf(stderr, "{%s/%s:%d}", fname->getDirectory().data(), fname->getFilename().data(), loc->getLine());
                        // Get callee
                        const DISubprogram *sub = callee->getSubprogram();
                        if(sub)
                        {
                            fprintf(stderr, "|{%s/%s:%d}", sub->getDirectory().data(), sub->getFilename().data(), sub->getLine());
                        }
                    }
                    fprintf(stderr, "\n");
                }
                else if(!SecondPass){ 
                    if(!I.use_empty())
                        DefCallUse[callee].second += 1;
                    DefCallUse[callee].first += 1;
                    //errs() << "502:" << F->getName() << " " << DefCallUse[F].first << " " << DefCallUse[F].second << "\n";
                }
            }
        }
    }

    void buildLocalVMap(Function &F)
    {
        string func_name = F.getName().str();
        for(auto &BB: F)
            for(auto &I:BB) 
            {
                
            }
    }
    
    bool runOnModule(Module &M) override
    {
        bool all = !target.compare("all");
        Module::FunctionListType &F_list = M.getFunctionList();
        for(Function &F: F_list) {
            detectTransientValue(F, false);
        }
        for(Function &F: F_list) {
            detectTransientValue(F, true);
        }

        return false;
    }
}; // end of struct Liveness
} // end of anonymous namespace

char TransientLiveness::ID = 0;
static RegisterPass<TransientLiveness> X("TransientLiveness", "Local Liveness Analysis",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);
