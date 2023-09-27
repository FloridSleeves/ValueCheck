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
#include "llvm/IR/Operator.h"
#include <iostream>
#include <cxxabi.h>
using namespace llvm;
using namespace std;
#define DEBUG_TYPE "Liveness"
#define STDERR_FILENO 2

int pointer_alias_counter = 0;
int value_alias_counter = 0;
int resource_cleanup_counter = 0;
int inline_variable_counter = 0;

bool unused_flag = true;

inline std::string demangle(const char* name)
{
    int status = -1;
    std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free   };
    return (status == 0) ? res.get() : std::string(name);
}


typedef string Field;

namespace
{

    typedef struct{
        Value *v;
        unsigned offset;
    } StructValue;

    bool operator==(const StructValue&a, const StructValue& b)
    {
        return (a.v == b.v) && (a.offset == b.offset);
    }
    bool operator<(const StructValue&a, const StructValue& b)
    {
        return (a.v < b.v) || ((a.v == b.v) && (a.offset < b.offset));
    }
    bool operator>(const StructValue&a, const StructValue& b)
    {
        return (a.v > b.v) || ((a.v == b.v) && (a.offset > b.offset));
    }

struct LocalLiveness : public ModulePass
{
    string func_name = "test";
    static char ID;
    map<string, set<pair<StructValue, Instruction*>> > sum_unused_map;
    map<string, map<Value*, DILocalVariable*>> localv_map;
    //const TargetLibraryInfo TLI;
    LocalLiveness() : ModulePass(ID) {
    }
    
    Value* stripBitcasts(Value *v)
    {
        if(BitCastOperator *bc_v = dyn_cast<BitCastOperator>(v))
        {
            // strip cast
            return bc_v->stripPointerCastsAndAliases();
        }
        else if(BitCastInst *bc_v = dyn_cast<BitCastInst>(v))
        {
            return bc_v->getOperand(0);
        }
        else if(CastInst *bc_v = dyn_cast<CastInst>(v))
        {
            return bc_v->getOperand(0);
        }
        return v;
    }

    bool castToStruct(Value *v, StructValue &sv)
    {
        PointerType *v_ty_p = dyn_cast<PointerType>(v->getType());
        if(v_ty_p == NULL)
            return false;
        Type *v_ty = v_ty_p->getElementType();
        if(StructType *v_sty = dyn_cast<StructType>(v_ty))
        {
            // struct
            sv.v = v;
            sv.offset = -1;
            return true;
        }
        return false;
    }

    bool castToField(Value *v, StructValue &sv)
    {
        Type *v_ty = v->getType();
        if(GEPOperator *gep = dyn_cast<GEPOperator>(v))
        {
            Type *src_ty = gep->getSourceElementType();
            if(StructType *src_sty = dyn_cast<StructType>(src_ty))
            {
                sv.v = gep->getOperand(0);
                if(isa<GlobalVariable>(sv.v))
                    return false;
                unsigned num_index = gep->getNumIndices();
                if(num_index == 2)
                {
                    if(ConstantInt *ci = dyn_cast<ConstantInt>(gep->getOperand(2)))
                    {
                        sv.offset = ci->getZExtValue();
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void initialize(vector<BasicBlock *> &BBList, string f_name, vector<set<StructValue>> &UEVar, vector<set<StructValue>> &VarKill, bool print_flag)
    {
        for (int i = 0; i < BBList.size(); ++i)
        {
            for (auto &inst : *BBList[i])
            {
                if (inst.getOpcode() == Instruction::Load)
                {
                    Value *v = stripBitcasts(inst.getOperand(0));//->stripPointerCastsAndAliases();
                    StructValue sv;
                    if (castToStruct(v, sv))  
                    {
                        // this is impossible
                        // add all offset to UEVar
                        StructType *v_sty = dyn_cast<StructType>(dyn_cast<PointerType>(v->getType())->getElementType());
                        unsigned field_num = v_sty->getNumElements();
                        for(unsigned fi = 0; fi < field_num; fi++)
                        {
                            sv.offset = fi;
                            if(VarKill[i].find(sv) == VarKill[i].end())
                                UEVar[i].insert(sv);
                        }
                    }
                    else if(castToField(v, sv))
                    {
                        if(VarKill[i].find(sv) == VarKill[i].end())
                        {
                            UEVar[i].insert(sv);
                        }
                    }
                }
                if (inst.getOpcode() == Instruction::Store)
                {
                    Value *v = inst.getOperand(1);//->stripPointerCastsAndAliases();
                    StructValue sv;
                    // we do not count the whole struct assignment
                    if(castToField(v, sv))
                    {
                        if(VarKill[i].find(sv) == VarKill[i].end())
                        {
                            VarKill[i].insert(sv);
                        }
                    }
                }
                if (CallBase *call_inst = dyn_cast<CallBase>(&inst))
                {
                    Function *callee = call_inst->getCalledFunction();
                    if(callee == NULL)
                        continue;
                    string callee_name = callee->getName().str();
                    // record variable name
                    if(callee_name.compare("llvm.dbg.declare") == 0)
                    {
                        // declare local variable
                        DbgDeclareInst *dbgI = dyn_cast<DbgDeclareInst>(call_inst);
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

    bool computeLiveOut(int bbIndex, map<BasicBlock *, int> &BBMap, vector<BasicBlock *> &BBList, vector<set<StructValue>> &UEVar, vector<set<StructValue>> &VarKill, vector<set<StructValue>> &LiveIn, vector<set<StructValue>> &LiveOut, bool print_flag)
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

    void computeDef(set<pair<StructValue, Instruction*>> &UnusedDef, set<StructValue> &Def, map<BasicBlock*, int> &BBMap, vector<BasicBlock *> &BBList,vector<set<StructValue>> &VarKill, vector<set<StructValue>>&LiveOut, string curr_name, bool print_flag)
    {
        // Get unuseddef 
        for(auto &BB : BBList)
        {
            int idx = BBMap[BB];
            // go through the bb reversely and find each def inst 
            // if it's not in liveout
            // record it into UnusedDef
            set<StructValue> curr_liveout(LiveOut[idx].begin(), LiveOut[idx].end());
            for(auto I = BB->rbegin(), EI = BB->rend(); I!= EI; I++)
            {
                // check whether generate a def (and the def is unused in callee)
                switch(I->getOpcode())
                {
                    case Instruction::Store:
                    {
                        // if a def
                        Value *v = I->getOperand(1)->stripPointerCastsAndAliases();
                        StructValue sv;
                        if(castToField(v, sv))
                        {
                            // check curr_liveout
                            if(curr_liveout.find(sv) == curr_liveout.end())
                            {
                                UnusedDef.insert(make_pair(sv, &(*I)));
                            }
                            auto it = curr_liveout.find(sv);
                            if(it!= curr_liveout.end())
                            {
                                curr_liveout.erase(it);
                            }
                        }
                        break;
                    }
                    case Instruction::Load:
                    {
                        Value *v = stripBitcasts(I->getOperand(0));
                        StructValue sv;
                        if (castToStruct(v, sv))  
                        {
                            // this is impossible
                            // add all offset to UEVar
                            StructType *v_sty = dyn_cast<StructType>(dyn_cast<PointerType>(v->getType())->getElementType());
                            unsigned field_num = v_sty->getNumElements();
                            for(unsigned fi = 0; fi < field_num; fi++)
                            {
                                sv.offset = fi;
                                curr_liveout.insert(sv);
                            }
                        }
                        if(castToField(v, sv))
                        {
                            // add to liveness
                            curr_liveout.insert(sv);
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
        if(f_name.find("llvm") != string::npos)
        {
            return;
        }
        for (auto &B : *F)
        {
            for (auto &inst : B)
            {
                if (inst.getOpcode() == Instruction::Load)
                {
                }
                if (inst.getOpcode() == Instruction::Store)
                {
                    Value *v =   stripBitcasts(inst.getOperand(0)->stripPointerCastsAndAliases());
                    if(isa<AllocaInst>(v))
                    {
                        point_to_set.insert(v);
                    }
                }
                if (inst.getOpcode() == Instruction::Call || inst.getOpcode() == Instruction::Invoke)
                {
                    CallBase *call_inst = cast<CallBase>(&inst);
                    Function *callee = call_inst->getCalledFunction();
                    if(callee && callee->getName().find("llvm.") != string::npos)
                    {
                        if(callee->getName().find("memcpy") != string::npos)
                        {
                            // add memcpy src into point-to
                            Value *src = stripBitcasts(call_inst->getArgOperand(1)->stripPointerCastsAndAliases());
                            point_to_set.insert(src);
                        }
                        continue;
                    }
                    for(auto arg_i = call_inst->arg_begin(), arg_e = call_inst->arg_end(); arg_i != arg_e; arg_i++)
                    {
                        Value *orig_v = stripBitcasts(arg_i->get()->stripPointerCastsAndAliases());
                        if(isa<AllocaInst>(orig_v))
                        {
                            point_to_set.insert(orig_v);
                        }
                    }
                }
                if (inst.getOpcode() == Instruction::Select)
                {
                    SelectInst *selectI = dyn_cast<SelectInst>(&inst);
                    Value *vlist[3];
                    vlist[0] = stripBitcasts(selectI->getCondition()->stripPointerCastsAndAliases());
                    vlist[1] = stripBitcasts(selectI->getTrueValue()->stripPointerCastsAndAliases());
                    vlist[2] = stripBitcasts(selectI->getFalseValue()->stripPointerCastsAndAliases());
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
                        Value *incomingV = stripBitcasts(phiI->getIncomingValue(j)->stripPointerCastsAndAliases());
                        if(isa<AllocaInst>(incomingV))
                        {
                            point_to_set.insert(incomingV);
                        }
                    }
                }
            }
        }
        return;
    }

    bool traverseFunc(Function *F, bool print_flag)
    {
        set<StructValue> Def, Use;
        string f_name = F->getName().str();
        
        /* DEBUG
        if(f_name.compare("_ZN6Format5Token5parseEPKcPNS_7QuotingE") != 0)
            return false;
        //DEBUG END*/
        
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
        auto uk_map = make_pair(vector<set<StructValue>>(count), vector<set<StructValue>>(count));
        // initialize: print out use/def/call 
        initialize(BBList, f_name, uk_map.first, uk_map.second, print_flag);

        vector<set<StructValue>> LiveIn(count);
        vector<set<StructValue>> LiveOut(count);

        // iterative algorithm to compute LiveOut
        bool flag = true;
        int bblist_size = BBList.size();
        // compute livein/liveout based on uvar and vkill
        while (flag)
        {
            flag = false;
            for (int i = 0; i < bblist_size; ++i)
            {
                set<StructValue> prev = LiveIn[bblist_size-i-1];
                computeLiveOut(bblist_size - i - 1, BBMap, BBList, uk_map.first, uk_map.second, LiveIn, LiveOut, print_flag);
                if (
                        !std::equal(
                            prev.begin(), prev.end(), 
                            LiveIn[bblist_size- i-1].begin(), LiveIn[bblist_size - i - 1].end())
                    )
                {
                    flag = true;
                }    
            }
        }
        // compute external use based on livein[0]
        set<pair<StructValue, Instruction*>> UnusedDef;
        // compute unuseddef/def
        computeDef(UnusedDef, Def, BBMap, BBList, uk_map.second, LiveOut, f_name, print_flag);
        set<Value*> point_to_set;
        addPointTo(F, point_to_set);
        for(auto &v: UnusedDef)
        {
            if(point_to_set.find(v.first.v) != point_to_set.end())
            {
                if(print_flag) 
                {
                    DILocalVariable *localv = localv_map[f_name][v.first.v];
                    if(localv)
                        fprintf(stderr, "!Pointer Alias: %s\n", localv->getName().data());
                }
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
        //I->print(errs());
        if(Constant *c = dyn_cast<Constant>(cast<StoreInst>(I)->getValueOperand()))
        {
            c = c->stripPointerCasts();
            if(c->isNullValue() || c->isZeroValue())
                return true;
        }
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

        if(v->hasOneUse()|| isa<Constant>(v))
        {
            return false;
        }
        return true;
    }
    
    bool checkReturn(Function *f, Value *v)
    {
        if(f->arg_size() && f->hasParamAttribute(0, Attribute::StructRet))
        {
            Value *arg0 = dyn_cast<Value>(f->getArg(0));
            if(v == arg0)
                return true;
        }
        return false;
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

    void computeDUSig(Module &M)
    {

    }

    bool runOnModule(Module &M) override
    {

        Module::FunctionListType &F_list = M.getFunctionList();
        //first Pass, do not get into any func traverse
        errs() << "First Pass...\n";

        for(Function &F: F_list)
        {
            //if(F.getName().compare("LZ4HC_compress_optimal")==0)
            traverseFunc(&F, true);
        }

       if(unused_flag)
        {
            int count_alias = 0;
            fprintf(stderr,  "Final Unused:\n");
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
                    Function *func = v.second->getFunction();
                    DILocalVariable *locv = localv_map[item.first][v.first.v];
                    if(locv == NULL)
                        continue;
                    string vname = locv->getName().str();
                    if(checkValueAlias(func, v.second))
                    {
                        //fprintf(stderr, "!Value Alias: %s\n", vname.data());
                        value_alias_counter += 1;
                        continue;
                    }
                    if(checkInlineVariable(func, v.first.v))
                    {
                        //fprintf(stderr, "!Inline Variable: %s\n", vname.data());
                        inline_variable_counter += 1;
                        continue;
                    }
                    if(checkReturn(func, v.first.v))
                    {
                        //fprintf(stderr, "!Return Variable: %s\n", vname.data());
                        continue;
                    }
                    if(checkZeroDef(v.second))
                    {
                        //fprintf(stderr,  "[ResourceCleanup]");
                        resource_cleanup_counter += 1;
                    }

                    if(vname.empty() || vname.compare("_dbg_level") == 0)
                        continue;
                    fprintf(stderr, "%s:\n", orig_fname.c_str());
                    fprintf(stderr,  "@%s:%d", vname.c_str(), v.first.offset);
                    DIFile *fname = dyn_cast<DIFile>(locv->getFile());
                    const DebugLoc &loc = v.second->getDebugLoc();
                    if(fname && loc)
                        fprintf(stderr, "{%s/%s:%d}\n", fname->getDirectory().data(), fname->getFilename().data(), loc.getLine());

                    /*fprintf(stderr, "\n[");
                    v.second->print(errs());
                    fprintf(stderr, "] (%d)\n", v.first.v->getNumUses());*/
                }
            }
        }
        /*if(du_flag)
        {
            computeDUSig(M);
        }*/
        fprintf(stderr, "[FP Stat: Value Alias: %d, Pointer Alias: %d, Resource Cleanup: %d, Inline Variable: %d ]\n", value_alias_counter, pointer_alias_counter, resource_cleanup_counter, inline_variable_counter);
        return false;
    }
}; // end of struct Liveness
} // end of anonymous namespace

char LocalLiveness::ID = 0;
static RegisterPass<LocalLiveness> X("FieldLocalLiveness", "Local Liveness Analysis",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);
