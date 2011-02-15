#include "absorpc.h"
#include "math/spline.h"

cm_Absorption_Coefficient_Reg::cm_Absorption_Coefficient_Reg()  {
  entries.Add("Ac", _cm_absorpc_Ac);
  entries.Add("Ag", _cm_absorpc_Ag);
  entries.Add("Al", _cm_absorpc_Al);
  entries.Add("Ar", _cm_absorpc_Ar);
  entries.Add("As", _cm_absorpc_As);
  entries.Add("At", _cm_absorpc_At);
  entries.Add("Au", _cm_absorpc_Au);
  entries.Add("B", _cm_absorpc_B);
  entries.Add("Ba", _cm_absorpc_Ba);
  entries.Add("Be", _cm_absorpc_Be);
  entries.Add("Bi", _cm_absorpc_Bi);
  entries.Add("Br", _cm_absorpc_Br);
  entries.Add("C", _cm_absorpc_C);
  entries.Add("Ca", _cm_absorpc_Ca);
  entries.Add("Cd", _cm_absorpc_Cd);
  entries.Add("Ce", _cm_absorpc_Ce);
  entries.Add("Cl", _cm_absorpc_Cl);
  entries.Add("Co", _cm_absorpc_Co);
  entries.Add("Cr", _cm_absorpc_Cr);
  entries.Add("Cs", _cm_absorpc_Cs);
  entries.Add("Cu", _cm_absorpc_Cu);
  entries.Add("Dy", _cm_absorpc_Dy);
  entries.Add("Er", _cm_absorpc_Er);
  entries.Add("Eu", _cm_absorpc_Eu);
  entries.Add("F", _cm_absorpc_F);
  entries.Add("Fe", _cm_absorpc_Fe);
  entries.Add("Fr", _cm_absorpc_Fr);
  entries.Add("Ga", _cm_absorpc_Ga);
  entries.Add("Gd", _cm_absorpc_Gd);
  entries.Add("Ge", _cm_absorpc_Ge);
  entries.Add("H", _cm_absorpc_H);
  entries.Add("He", _cm_absorpc_He);
  entries.Add("Hf", _cm_absorpc_Hf);
  entries.Add("Hg", _cm_absorpc_Hg);
  entries.Add("Ho", _cm_absorpc_Ho);
  entries.Add("I", _cm_absorpc_I);
  entries.Add("In", _cm_absorpc_In);
  entries.Add("Ir", _cm_absorpc_Ir);
  entries.Add("K", _cm_absorpc_K);
  entries.Add("Kr", _cm_absorpc_Kr);
  entries.Add("La", _cm_absorpc_La);
  entries.Add("Li", _cm_absorpc_Li);
  entries.Add("Lu", _cm_absorpc_Lu);
  entries.Add("Mg", _cm_absorpc_Mg);
  entries.Add("Mn", _cm_absorpc_Mn);
  entries.Add("Mo", _cm_absorpc_Mo);
  entries.Add("N", _cm_absorpc_N);
  entries.Add("Na", _cm_absorpc_Na);
  entries.Add("Nb", _cm_absorpc_Nb);
  entries.Add("Nd", _cm_absorpc_Nd);
  entries.Add("Ne", _cm_absorpc_Ne);
  entries.Add("Ni", _cm_absorpc_Ni);
  entries.Add("O", _cm_absorpc_O);
  entries.Add("Os", _cm_absorpc_Os);
  entries.Add("P", _cm_absorpc_P);
  entries.Add("Pa", _cm_absorpc_Pa);
  entries.Add("Pb", _cm_absorpc_Pb);
  entries.Add("Pd", _cm_absorpc_Pd);
  entries.Add("Pm", _cm_absorpc_Pm);
  entries.Add("Po", _cm_absorpc_Po);
  entries.Add("Pr", _cm_absorpc_Pr);
  entries.Add("Pt", _cm_absorpc_Pt);
  entries.Add("Ra", _cm_absorpc_Ra);
  entries.Add("Rb", _cm_absorpc_Rb);
  entries.Add("Re", _cm_absorpc_Re);
  entries.Add("Rh", _cm_absorpc_Rh);
  entries.Add("Rn", _cm_absorpc_Rn);
  entries.Add("Ru", _cm_absorpc_Ru);
  entries.Add("S", _cm_absorpc_S);
  entries.Add("Sb", _cm_absorpc_Sb);
  entries.Add("Sc", _cm_absorpc_Sc);
  entries.Add("Se", _cm_absorpc_Se);
  entries.Add("Si", _cm_absorpc_Si);
  entries.Add("Sm", _cm_absorpc_Sm);
  entries.Add("Sn", _cm_absorpc_Sn);
  entries.Add("Sr", _cm_absorpc_Sr);
  entries.Add("Ta", _cm_absorpc_Ta);
  entries.Add("Tb", _cm_absorpc_Tb);
  entries.Add("Tc", _cm_absorpc_Tc);
  entries.Add("Te", _cm_absorpc_Te);
  entries.Add("Th", _cm_absorpc_Th);
  entries.Add("Ti", _cm_absorpc_Ti);
  entries.Add("Tl", _cm_absorpc_Tl);
  entries.Add("Tm", _cm_absorpc_Tm);
  entries.Add("U", _cm_absorpc_U);
  entries.Add("V", _cm_absorpc_V);
  entries.Add("W", _cm_absorpc_W);
  entries.Add("Xe", _cm_absorpc_Xe);
  entries.Add("Y", _cm_absorpc_Y);
  entries.Add("Yb", _cm_absorpc_Yb);
  entries.Add("Zn", _cm_absorpc_Zn);
  entries.Add("Zr", _cm_absorpc_Zr);
}

double cm_Absorption_Coefficient_Reg::_CalcForE(double eV,
  const cm_Absorption_Coefficient* ac, double (cm_Absorption_Coefficient::*f)() const) const {
  eV /= 1e6;
  double t_ev = eV;
  long k = 1000; // 'cut' energy to 3 significant digits
  while( int(t_ev) == 0 )  {
    t_ev *= 10;
    k *= 10;
  }
  while( int (t_ev) > 10 )  {
    t_ev /= 10;
    k /= 10;
  }
  if( k != 0 )
    eV = double((long)(eV*k))/k;
  if( ac == NULL )
    throw TFunctionFailedException(__OlxSourceInfo, "undefined absorption data");
  if( eV < ac[0].energy )
    throw TInvalidArgumentException(__OlxSourceInfo, "energy is out of range");
  if( eV == ac[0].energy )
    return ac[0].muen_over_rho;
  size_t cnt = 0;
  const cm_Absorption_Coefficient* _ac = ac;
  while( ac->energy < eV && ac->energy != 0 )  {
    cnt++;
    ac++;
  }
  if( ac->energy == 0 )
    throw TInvalidArgumentException(__OlxSourceInfo, "energy is out of range");
  // matches the entry?
  if( olx_abs(ac->energy-eV) < 1e-3/k )
    return (ac->*f)();
  // go left
  size_t l_cnt = cnt-1;
  while( l_cnt > 0 && (cnt-l_cnt) < 4 && _ac[l_cnt].energy != 0 )  {
    if( _ac[l_cnt-1].energy == _ac[l_cnt].energy )  // absorption edge
      break;
    l_cnt--;
  }
  // go right
  size_t r_cnt = cnt+1;
  while( (r_cnt-cnt) < 4 && _ac[r_cnt].energy != 0 )  {
    if( _ac[r_cnt+1].energy == _ac[r_cnt].energy )  // absorption edge
      break;
    r_cnt++;
  }
  if( (r_cnt-l_cnt) >= 5 )  {  // use spline interpolation
    math::spline::Spline3<double> s;
    s.x.Resize(r_cnt-l_cnt);
    s.y.Resize(r_cnt-l_cnt);
    for( size_t i=l_cnt; i < r_cnt; i++ )  {
      s.x(i-l_cnt) = _ac[i].energy;
      s.y(i-l_cnt) = (_ac[i].*f)();
    }
    return math::spline::Builder<double>::akima(s).interpolate(eV);
  }
  if( ac->energy > eV )  {    
    if( (ac->*f)() == ((ac-1)->*f)() )
      return ac->muen_over_rho;
    const double k = (eV-(ac-1)->energy)/(ac->energy - (ac-1)->energy);
    return ((ac-1)->*f)() + k*((ac->*f)()-((ac-1)->*f)());
  }
  else if( ac->energy == eV )
    return ac->muen_over_rho;
  throw TFunctionFailedException(__OlxSourceInfo, "cannot happen");
}
