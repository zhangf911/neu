/*================================= Neu =================================
 
 Copyright (c) 2013-2014, Andrometa LLC
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 
 3. Neither the name of the copyright holder nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 =======================================================================*/

#include <neu/NMLGenerator.h>

using namespace std;
using namespace neu;

#include <neu/NObject.h>

namespace{
  
  enum SymbolKey{
    
  };
  
  enum FunctionKey{
    FKEY_NO_KEY,
    FKEY_Var_1,
    FKEY_Var_2,
    FKEY_Throw_2,
    FKEY_Add_2,
    FKEY_Sub_2,
    FKEY_Mul_2,
    FKEY_Div_2,
    FKEY_Mod_2,
    FKEY_LT_2,
    FKEY_GT_2,
    FKEY_LE_2,
    FKEY_GE_2,
    FKEY_EQ_2,
    FKEY_NE_2,
    FKEY_Not_1,
    FKEY_And_2,
    FKEY_Or_2,
    FKEY_Inc_1,
    FKEY_PostInc_1,
    FKEY_Dec_1,
    FKEY_PostDec_1,
    FKEY_AddBy_2,
    FKEY_SubBy_2,
    FKEY_MulBy_2,
    FKEY_DivBy_2,
    FKEY_ModBy_2,
    FKEY_Neg_1,
    FKEY_Pow_2,
    FKEY_Set_2
  };
  
  typedef NMap<nstr, SymbolKey> SymbolMap;
  
  typedef NMap<pair<nstr, int>, FunctionKey> FunctionMap;
  
  static SymbolMap _symbolMap;
  
  static FunctionMap _functionMap;
  
  static void _initSymbolMap(){
    
  }
  
  static void _initFunctionMap(){
    _functionMap[{"Var", 1}] = FKEY_Var_1;
    _functionMap[{"Var", 2}] = FKEY_Var_2;
    _functionMap[{"Throw", 2}] = FKEY_Throw_2;
    _functionMap[{"Add", 2}] = FKEY_Add_2;
    _functionMap[{"Sub", 2}] = FKEY_Sub_2;
    _functionMap[{"Mul", 2}] = FKEY_Mul_2;
    _functionMap[{"Div", 2}] = FKEY_Div_2;
    _functionMap[{"Pow", 2}] = FKEY_Pow_2;
    _functionMap[{"Mod", 2}] = FKEY_Mod_2;
    _functionMap[{"Set", 2}] = FKEY_Set_2;
    _functionMap[{"LT", 2}] = FKEY_LT_2;
    _functionMap[{"GT", 2}] = FKEY_GT_2;
    _functionMap[{"LE", 2}] = FKEY_LE_2;
    _functionMap[{"GE", 2}] = FKEY_GE_2;
    _functionMap[{"EQ", 2}] = FKEY_EQ_2;
    _functionMap[{"NE", 2}] = FKEY_NE_2;
    _functionMap[{"Not", 1}] = FKEY_Not_1;
    _functionMap[{"And", 2}] = FKEY_And_2;
    _functionMap[{"Or", 2}] = FKEY_Or_2;
    _functionMap[{"Inc", 1}] = FKEY_Inc_1;
    _functionMap[{"PostInc", 1}] = FKEY_PostInc_1;
    _functionMap[{"Dec", 1}] = FKEY_Dec_1;
    _functionMap[{"PostDec", 1}] = FKEY_PostDec_1;
    _functionMap[{"AddBy", 2}] = FKEY_AddBy_2;
    _functionMap[{"SubBy", 2}] = FKEY_SubBy_2;
    _functionMap[{"MulBy", 2}] = FKEY_MulBy_2;
    _functionMap[{"DivBy", 2}] = FKEY_DivBy_2;
    _functionMap[{"ModBy", 2}] = FKEY_ModBy_2;
    _functionMap[{"Neg", 1}] = FKEY_Neg_1;
  };
  
  class _FunctionMapLoader{
  public:
    _FunctionMapLoader(){
      _initSymbolMap();
      _initFunctionMap();
    }
  };
  
  static _FunctionMapLoader* _functionMapLoader = new _FunctionMapLoader;
  
} // end namespace

namespace neu{
  
  class NMLGenerator_{
  public:
    NMLGenerator_(NMLGenerator* o)
    : o_(o){
      
    }
    
    ~NMLGenerator_(){
      
    }
    
    void generate(ostream& ostr, const nvar& v){
      emitExpression(ostr, "", v);
    }
    
    FunctionKey getFunctionKey(const nvar& f){
      FunctionMap::const_iterator itr =
      _functionMap.find(make_pair(f.str(), f.size()));
      
      if(itr == _functionMap.end()){
        itr = _functionMap.find(make_pair(f.str(), -1));
      }
      
      if(itr == _functionMap.end()){
        return FKEY_NO_KEY;
      }
      
      return itr->second;
    }
    
    void emitExpression(ostream& ostr,
                        const nstr& indent,
                        const nvar& v,
                        int prec=100){
      switch(v.type()){
        case nvar::Function:
          break;
        default:
          ostr << v;
          return;
      }
      
      FunctionKey key = getFunctionKey(v);
      
      switch(key){
        case FKEY_Add_2:{
          int p = NObject::precedence(v);
          
          if(p > prec){
            ostr << "(";
          }
          
          emitExpression(ostr, indent, v[0], p);
          ostr << " + ";
          emitExpression(ostr, indent, v[1], p);
          
          if(p > prec){
            ostr << ")";
          }
          break;
        }
        case FKEY_Sub_2:{
          int p = NObject::precedence(v);
          
          if(p > prec){
            ostr << "(";
          }
          
          emitExpression(ostr, indent, v[0], p);
          ostr << " - ";
          emitExpression(ostr, indent, v[1], p);
          
          if(p > prec){
            ostr << ")";
          }
          break;
        }
        case FKEY_Mul_2:{
          int p = NObject::precedence(v);
          
          if(p > prec){
            ostr << "(";
          }
          
          emitExpression(ostr, indent, v[0], p);
          ostr << " * ";
          emitExpression(ostr, indent, v[1], p);
          
          if(p > prec){
            ostr << ")";
          }
          break;
        }
        case FKEY_Div_2:{
          int p = NObject::precedence(v);
          
          if(p > prec){
            ostr << "(";
          }
          
          emitExpression(ostr, indent, v[0], p);
          ostr << " / ";
          emitExpression(ostr, indent, v[1], p);
          
          if(p > prec){
            ostr << ")";
          }
          break;
        }
        default:
          NERROR("[1] invalid function: " + v);
      }
    }
    
  private:
    NMLGenerator* o_;
  };
  
} // end namespace neu

NMLGenerator::NMLGenerator(){
  x_ = new NMLGenerator_(this);
}

NMLGenerator::~NMLGenerator(){
  delete x_;
}

void NMLGenerator::generate(std::ostream& ostr, const nvar& v){
  x_->generate(ostr, v);
}

nstr NMLGenerator::toStr(const nvar& v){
  NMLGenerator generator;
  
  stringstream ostr;
  generator.generate(ostr, v);
  
  return ostr.str();
}
