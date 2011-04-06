#include "symmparser.h"
#include "estack.h"
#include "estrlist.h"
#include "emath.h"
//---------------------------------------------------------------------------
const char TSymmParser::Axis[] = {'X','Y','Z'};
//..............................................................................
// Transforms standard SYMM operation (INS, CIF files) to matrix
bool TSymmParser::SymmToMatrix(const olxstr& S, smatd& M)  {
  olxstr p, p1, opr;
  double ratio;
  int op, index;
  bool res = true;
  str_stack stack;
  M.Null();
  TStrList toks(S.ToUpperCase().DeleteChars(' '), ',');
  if( toks.Count() != 3 )
    throw TFunctionFailedException(__OlxSourceInfo,
         olxstr("Operation sign is missing or operation is incomplete while parsing \"") << S << '\"');

  for( int i=0; i < 3; i++ )  {
    stack.LoadFromExpression(toks[i]);
    p = stack.Pop();
    op = IsAxis(p);
    if( op < 0 )  {
      p1 = stack.Pop();
      if( stack.IsEmpty() )  {
        res = false;  break;
      }
      if( p1.CharAt(0) == '/' )  {
        p1 = stack.Pop();
        ratio = p1.ToDouble()/p.ToDouble();
        p1 = stack.Pop();
        if( p1 == '+' )  
          M.t[i] = ratio;
        else if( p1 == '-' )
          M.t[i] = -ratio;
      }
      else
        M.t[i] = (p1 == '-' ? -p.ToDouble() : p.ToDouble());

      p = stack.Pop();
    }
next_oper:
    if( abs(p.CharAt(0)-'Z') > 2 )  {
      res = false;
      break;
    }
    index = 2 - 'Z' + p.CharAt(0);
    M.r[i][index] = 1;
    if( stack.IsEmpty() )  continue;
    p = stack.Pop();
    if( p == '-' )
      M.r[i][index] = -1;  // inversion
    if( stack.IsEmpty() )  continue;
    p = stack.Pop();
    op = IsAxis(p);
    if( op < 0 )  {
      if( stack.IsEmpty() )  {
        M.t[i] = p.ToDouble();  // translation
        continue;
      }
      if( stack.Count() == 1 )  {
        p = stack.Pop() << p;
        M.t[i] = p.ToDouble();  // translation
        continue;
      }
      opr = stack.Pop();  // should be '/'
      if( opr != '/' )  {
        res = false;  
        break;
      }
      p1 = stack.Pop();
      ratio = p1.ToDouble()/p.ToDouble();
      if( !stack.IsEmpty() )  {
        p = stack.Pop();
        if( p == '-' )
          ratio *= -1;
        else if( p == '+' )
          ;
        else
          stack.Push( p );
      }
      M.t[i] = ratio;  // translation
    }
    else
      goto next_oper;
  }
  if( res == false )
    throw TFunctionFailedException(__OlxSourceInfo,
         olxstr("Operation sign is missing or operation is incomplete while parsing \"") << S << '\"');
  // normalise the translations...
  for( int i=0; i < 3; i++ )  {
    if( M.t[i] == 0 )  continue;
    int v = olx_abs(olx_round(M.t[i]*12)), base = 12;
    int denom = olx_gcd(v, base);
    if( denom != 1 )  {
      v /= denom;
      base /= denom;
    }
    M.t[i] = (M.t[i] < 0 ? -v : v);
    if( base != 1 )
      M.t[i] /= base;
  }
  return true;
}
//..............................................................................
olxstr TSymmParser::MatrixToSymmEx(const mat3i& M)  {
  olxstr T, T1;
  for( int j=0; j < 3; j ++ )  {
    if( j != 0 )
      T << ',';
    for( int i=0; i < 3; i ++ )  {
      if( i == j )  {
        if( M[j][i] != 0 )  {
          T1 << olx_sign_char(M[j][i]);
          T1 << Axis[j];
        }
      }
      else if( M[j][i] != 0 )  {
        T1.Insert(Axis[i], 0);
        T1.Insert(olx_sign_char(M[j][i]), 0);
      }
    }
    T << T1;
    T1.SetLength(0);
  }
  return T;
}
//..............................................................................
bool TSymmParser::IsAbsSymm(const olxstr& s)  {
  if( s.Length() < 5 )  return false;
  if( s.IndexOf(',') != InvalidIndex )  { // asbolute representation
    smatd m;
    try  {
      SymmToMatrix(s, m);
      return true;
    }
    catch(...)  {  return false;  }
  }
  return false;
}
//..............................................................................
bool TSymmParser::IsRelSymm(const olxstr& s)  {
  if( s.Length() < 4 )  return false;
  const size_t ui = s.IndexOf('_');
  if( ui != InvalidIndex )  {
    const size_t r = s.Length()-ui-1;
    if( !(r == 3 || r == 6) )
      return false;
    for( size_t i=0; i < s.Length(); i++ )  {
      if( i == ui )  continue;
      if( !olxstr::o_isdigit(s.CharAt(i)) )
        return false;
    }
  }
  else  {
    if( s.Length() < 4 || s.Length() > 9 )
      return false;
    for( size_t i=0; i < s.Length(); i++ )  {
      if( !olxstr::o_isdigit(s.CharAt(i)) )
        return false;
    }
  }
  return true;
}
//..............................................................................
bool TSymmParser::IsSymm(const olxstr& s)  {
  if( !IsRelSymm(s) )
    return IsAbsSymm(s);
  return true;
}
//..............................................................................

