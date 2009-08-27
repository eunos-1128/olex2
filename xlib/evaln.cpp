#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <math.h>
//#include <mem.h>
#include "log.h"
#include "evaln.h"
#include "bapp.h"
#include "emath.h"

//---------------------------------------------------------------------------
TSOperation::TSOperation( TSOperation *P, TStrPObjList<olxstr, TSOperation*> *Vars,
    TStrList *Funcs, TDoubleList* IVars)  {
  ToCalc = NULL;
  Operation = fNone;
  P2 = P1 = 0;
  ChSig = Expandable = false;
  Function = fNone;
  Value = 0;
  Left = Right = NULL;
  VariableIndex = -1;
  Parent = P;
  IVariables = IVars;
  Variables = Vars;
  Functions = Funcs;
}
TSOperation::~TSOperation()  {
  if( Right )   delete Right;
  if( ToCalc )  delete ToCalc;
}

int TSOperation::LoadFromExpression(const olxstr &Exp)  {
  int ob=0, cb=0;
  for( int i=0; i < Exp.Length(); i++ )  {
    if( Exp[i] == '(' )  ob++;
    if( Exp[i] == ')' )  cb++;
  }

  if( ob != cb )
    throw TFunctionFailedException(__OlxSourceInfo, "brackets number do not match");

  short op;
  olxstr o, f;
  bool NewOperation = false;
  TSOperation *Opr;
  if( !Exp.IsEmpty() )  {
    Opr = new TSOperation(this, Variables, Functions, IVariables);
    ToCalc = Opr;
    for( int i=0; i < Exp.Length(); i++ )  {
      switch( Exp[i] )  {
        case '-':    NewOperation = true;    op = fMinus;      goto new_Op;
        case '+':    NewOperation = true;    op = fPlus;       goto new_Op;
        case ' ':    break;
        case '*':    NewOperation = true;    op = fMultiply;   goto new_Op;
        case '/':    NewOperation = true;    op = fDivide;     goto new_Op;
        case '^':    NewOperation = true;    op = fExt;        goto new_Op;
        case '%':    NewOperation = true;    op = fRemainder;  goto new_Op;
        case '(':
        {
          if( NewOperation )  Opr->Operation = op;
          Opr->Function = true;
          Opr->Expandable = true;
          Opr->Func = o;
          o = EmptyString;
          ob = 1;
          cb = 0;
          i++;
          while( cb != ob )  {
            if( Exp[i] == ')' )      cb++;
            if( Exp[i] == '(' )      ob++;
            if( cb == ob )        break;
            Opr->FExp << Exp[i];
            i++;
          }
          if( Opr->Func.IsEmpty() )
            Opr->Function = false;
          break;
        }
        default:
          if( NewOperation )  {
new_Op:
            if( !o.IsEmpty() )  {  
              Opr->Param = o;
              o = EmptyString;
            }
            else if( op == fMinus )  {
              Opr->ChSig = !Opr->ChSig; // subsequent -- :)
              NewOperation = false;
              break;
            }
            Opr->Right = new TSOperation(this, Variables, Functions, IVariables);
            Opr->Right->Left = Opr;
            Opr = Opr->Right;
            Opr->Operation = op;
            NewOperation = false;
            break;
          }
          o << Exp[i];
          break;
      }
    }
    if( !o.IsEmpty() )  Opr->Param = o;
  }
  if( ToCalc == NULL )  return 0;
  Opr = ToCalc;
  while( Opr ) {
    if( Opr->Expandable )  {
      Opr->LoadFromExpression(Opr->FExp);
      Opr->Expandable = false;
    }
    Opr = Opr->Right;
  }
  Opr = ToCalc;
  bool Defined;
  while( Opr )  {
    if( Opr->Function )  {
      Defined = false;
      o = Opr->Func.UpperCase();
      if(  o == "ABS" )  {
        Opr->Function = fAbs;
        Defined = true;
      }
      if( o == "COS" )  {
        Opr->Function = fCos;
        Defined = true;
      }
      if( o == "SIN" )  {
        Opr->Function = fSin;
        Defined = true;
      }
      if( !Defined )
        Functions->Add(Opr->Func);
      Opr = Opr->Right;
      continue;
    }
    if( !Opr->Param.IsEmpty() )  {
      if( Opr->Param.IsNumber() )
        Opr->P1 = Opr->Param.ToDouble();
      else  {
        int index = Variables->IndexOf(Opr->Param.UpperCase());
        if( index == -1 )  {
          Variables->Add(Opr->Param, Opr);
          Opr->VariableIndex = Variables->Count()-1;
        }
        else
          Opr->VariableIndex = index;
      }
    }
    Opr = Opr->Right;
  }
  return 0;
}
/***************************************************************************/
double TSOperation::Evaluate()  {
  TSOperation *Opr = ToCalc;
  if( Opr )  {
    Opr->MDCalculate();
    Opr->SSCalculate();
    Opr->Calculate();
    return Opr->Value;
  }
  return 0;
}
void TSOperation::Calculate()  {
  TSOperation *S = this;
  double Val = 0;
  while( S != NULL )  {
    Val += S->Value;
    S = S->Right;
  }
  Value = Val;
}
/***************************************************************************/
void TSOperation::SSCalculate()  {
  if( Left )  {
    if( Operation == fPlus )  {
      Value = Left->Value+Value;
      Left->Value = 0;
    }
    else if( Operation == fMinus )  {
      Value = Left->Value-Value;
      Left->Value = 0;
    }
    else if( Operation == fRemainder )  {
      Value = (int)Left->Value%(int)Value;
      Left->Value = 0;
    }
  }
  if( Right )
    Right->SSCalculate();
}
/***************************************************************************/
/***************************************************************************/
void TSOperation::MDCalculate()  {
  double V1;
  if( VariableIndex >= 0 && IVariables != NULL )
    P1 = (*IVariables)[VariableIndex];
//  else 
//    TBasicApp::GetLog() << (olxstr("TSOperation:: undefined variable: ") << Param);

  if( ToCalc )  {
    ToCalc->MDCalculate();
    ToCalc->SSCalculate();
    ToCalc->Calculate();
    V1 = ToCalc->Value;
    switch( Function )  {
      case fNone:
        break;
      case fAbs:
        V1 = olx_abs(V1);
        break;
      case fDefined:
        V1 = (*Evaluator)(V1, 0);
        break;
      case fCos:
        V1 = V1*M_PI/180;
        V1 = cos(V1);
        break;
      case fSin:
        V1 = V1*M_PI/180;
        V1 = sin(V1);
        break;
      default:
        throw TFunctionFailedException(__OlxSourceInfo, olxstr("undefined function: ") << Param);
    }
    Value = V1;
  }
  else  {
    if( Function == fDefined )
      Value = (*Evaluator)(P1, 0);
    else  {
      Value = P1;
      if( Func.Length() != 0 )
        throw TFunctionFailedException(__OlxSourceInfo, olxstr("undefined function: ") << Func);
    }
    if( ChSig ) Value = -Value;
  }

  if( Left )  {
    if( Operation == fDivide )  {
      if( Value )    
        Value = Left->Value/Value;
      else
        throw TDivException(__OlxSourceInfo);
      if( Left->Operation == fMinus )
        Value = -Value;
      Left->Value = 0;
    }
    else if( Operation == fMultiply )  {
      Value = Left->Value*Value;
      if( Left->Operation == fMinus )
        Value = -Value;
      Left->Value = 0;
    }
    else if( Operation == fExt )  {
      Value = pow(Left->Value, Value);
      if( Left->Operation == fMinus )
        Value = -Value;
      Left->Value = 0;
    }
  }
  if( Right )
    Right->MDCalculate();
}
/***************************************************************************/
/***************************************************************************/
